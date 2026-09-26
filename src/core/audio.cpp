#include "audio.h"
#include "settings.h"
#include "platform/audio/audio_backend.h"
#include <QDebug>
#include <mutex>
#include <memory>
namespace {
std::mutex mutex;
std::shared_ptr<qs::AudioBackend> backend;
std::shared_ptr<qs::AudioBackend> current(){std::lock_guard<std::mutex> lock(mutex);return backend;}
}
void Audio::init(){std::lock_guard<std::mutex> lock(mutex);if(!backend){backend=std::make_shared<qs::AudioBackend>();if(!backend->ready())qWarning().noquote()<<backend->error();}}
void Audio::quit(){std::shared_ptr<qs::AudioBackend> old;{std::lock_guard<std::mutex> lock(mutex);old=std::move(backend);}if(old)old->shutdown();}
void Audio::play(const QString &filename){auto b=current();if(b)b->play(filename,Config.EffectVolume);}
void Audio::stop(){auto b=current();if(b)b->stop();}
void Audio::playBGM(const QString &filename){auto b=current();if(b)b->playMusic(filename,Config.BGMVolume);}
void Audio::setBGMVolume(float v){auto b=current();if(b)b->setMusicVolume(v);}
void Audio::stopBGM(){auto b=current();if(b)b->stopMusic();}
QString Audio::getVersion(){return "miniaudio 0.11.21 / Vorbis 1.3.7";}
