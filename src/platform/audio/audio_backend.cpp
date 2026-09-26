#include "audio_backend.h"
#include <QFile>
#include <QThreadPool>
#include <QDebug>
#include <QSet>
#include <map>
#include <mutex>
#include <vector>
#include <cmath>
#include <vorbis/vorbisfile.h>
#include "miniaudio.h"
namespace qs {
struct Decoded { std::vector<float> pcm; unsigned channels=0, rate=0; QString error; };
static Decoded decode(const QString &path) {
 Decoded data; OggVorbis_File stream{};
 if(ov_fopen(QFile::encodeName(path).constData(),&stream)!=0) { data.error="cannot open Vorbis stream"; return data; }
 struct Close { OggVorbis_File *s; ~Close(){ov_clear(s);} } close{&stream};
 int section=0; float **pcm=nullptr;
 for(;;) {
  const long frames=ov_read_float(&stream,&pcm,4096,&section);
  if(frames<0) {data.error="Vorbis decode failure";break;}
  if(frames==0)break;
  const auto *info=ov_info(&stream,section);
  if(!info || info->channels<1 || info->channels>8 || info->rate<1) {data.error="unsupported audio format";break;}
  if(data.channels && (data.channels!=unsigned(info->channels) || data.rate!=unsigned(info->rate))) {data.error="changing format in chained audio";break;}
  data.channels=unsigned(info->channels); data.rate=unsigned(info->rate);
  if(data.pcm.size()+size_t(frames)*data.channels>64*1024*1024) {data.error="decoded resource exceeds 256 MiB";break;}
  for(long f=0;f<frames;f++)for(unsigned c=0;c<data.channels;c++)data.pcm.push_back(pcm[c][f]);
 }
 if(data.pcm.empty() && data.error.isEmpty()) data.error="empty audio";
 return data;
}
struct Voice {
 Decoded data; ma_audio_buffer buffer{}; ma_sound sound{}; bool bufferReady=false,soundReady=false;
 ~Voice(){if(soundReady)ma_sound_uninit(&sound);if(bufferReady)ma_audio_buffer_uninit(&buffer);}
 bool initialize(ma_engine *engine) {
  auto config=ma_audio_buffer_config_init(ma_format_f32,data.channels,data.pcm.size()/data.channels,data.pcm.data(),nullptr);
  config.sampleRate=data.rate;
  bufferReady=ma_audio_buffer_init(&config,&buffer)==MA_SUCCESS;
  if(!bufferReady)return false;
  soundReady=ma_sound_init_from_data_source(engine,&buffer,MA_SOUND_FLAG_NO_SPATIALIZATION,nullptr,&sound)==MA_SUCCESS;
  return soundReady;
 }
};
struct AudioBackend::Impl {
 mutable std::mutex mutex; QThreadPool pool; ma_engine engine{};
 bool ready=false,closing=false,noDevice=false;
 QString error; unsigned generation=0,musicGeneration=0;
 float musicVolume=1;
 QSet<QString> pending;
 std::map<QString,std::unique_ptr<Voice>> voices;
 std::unique_ptr<Voice> music;
};
AudioBackend::AudioBackend(bool noDevice):d(new Impl) {
 d->noDevice=noDevice; d->pool.setMaxThreadCount(2);
 auto config=ma_engine_config_init(); config.noDevice=noDevice; config.channels=2; config.sampleRate=48000;
 d->ready=ma_engine_init(&config,&d->engine)==MA_SUCCESS;
 if(!d->ready)d->error="Core Audio output initialization failed";
}
AudioBackend::~AudioBackend(){shutdown();}
bool AudioBackend::ready()const {std::lock_guard<std::mutex> lock(d->mutex);return d->ready&&!d->closing;}
QString AudioBackend::error()const {std::lock_guard<std::mutex> lock(d->mutex);return d->error;}
void AudioBackend::request(const QString &path,float volume,bool bgm) {
 std::lock_guard<std::mutex> lock(d->mutex);
 if(!d->ready||d->closing)return;
 if(bgm){d->musicGeneration++;d->music.reset();d->musicVolume=volume;}
 else {
  auto found=d->voices.find(path);
  if(found!=d->voices.end()) {
   if(!ma_sound_is_playing(&found->second->sound)){
    ma_sound_seek_to_pcm_frame(&found->second->sound,0);ma_sound_set_volume(&found->second->sound,volume);ma_sound_start(&found->second->sound);
   }
   return;
  }
  if(d->pending.contains(path))return;
  if(d->pending.size()+d->voices.size()>=100) {
   for(auto it=d->voices.begin();it!=d->voices.end();) {
    if(!ma_sound_is_playing(&it->second->sound))it=d->voices.erase(it);else ++it;
   }
   if(d->pending.size()+d->voices.size()>=100){qWarning()<<"Audio channel limit:"<<path;return;}
  }
  d->pending.insert(path);
 }
 const auto generation=d->generation, musicGeneration=d->musicGeneration;
 d->pool.start([this,path,volume,bgm,generation,musicGeneration]{
  auto data=decode(path);
  std::lock_guard<std::mutex> lock(d->mutex);
  if(d->closing||!d->ready||generation!=d->generation)return;
  if(bgm && musicGeneration!=d->musicGeneration)return;
  if(!bgm)d->pending.remove(path);
  if(!data.error.isEmpty()){d->error=path+": "+data.error;qWarning().noquote()<<d->error;return;}
  auto voice=std::make_unique<Voice>();voice->data=std::move(data);
  if(!voice->initialize(&d->engine)){d->error="Audio voice initialization failed: "+path;qWarning()<<d->error;return;}
  ma_sound_set_volume(&voice->sound,bgm?d->musicVolume:volume);
  ma_sound_set_looping(&voice->sound,bgm);
  ma_sound_start(&voice->sound);
  if(bgm)d->music=std::move(voice);else d->voices[path]=std::move(voice);
 });
}
void AudioBackend::play(const QString &p,float v){request(p,v,false);}
void AudioBackend::playMusic(const QString &p,float v){request(p,v,true);}
void AudioBackend::setMusicVolume(float v){std::lock_guard<std::mutex> lock(d->mutex);d->musicVolume=v;if(d->music)ma_sound_set_volume(&d->music->sound,v);}
void AudioBackend::stopMusic(){std::lock_guard<std::mutex> lock(d->mutex);d->musicGeneration++;d->music.reset();}
void AudioBackend::stop(){std::lock_guard<std::mutex> lock(d->mutex);d->generation++;d->musicGeneration++;d->pending.clear();d->voices.clear();d->music.reset();}
void AudioBackend::waitForDecoders(){d->pool.waitForDone();}
void AudioBackend::shutdown(){
 {std::lock_guard<std::mutex> lock(d->mutex);if(d->closing)return;d->closing=true;d->generation++;}
 d->pool.waitForDone();
 std::lock_guard<std::mutex> lock(d->mutex);d->voices.clear();d->music.reset();d->pending.clear();
 if(d->ready)ma_engine_uninit(&d->engine);d->ready=false;
}
int AudioBackend::voiceCount()const{std::lock_guard<std::mutex> lock(d->mutex);return int(d->voices.size());}
bool AudioBackend::musicPlaying()const{std::lock_guard<std::mutex> lock(d->mutex);return d->music&&ma_sound_is_playing(&d->music->sound);}
bool AudioBackend::render(unsigned frames){
 std::lock_guard<std::mutex> lock(d->mutex);if(!d->noDevice||!d->ready)return false;
 std::vector<float> pcm(size_t(frames)*2);if(ma_engine_read_pcm_frames(&d->engine,pcm.data(),frames,nullptr)!=MA_SUCCESS)return false;
 for(float sample:pcm)if(std::isfinite(sample)&&std::fabs(sample)>0.000001)return true;return false;
}
}
