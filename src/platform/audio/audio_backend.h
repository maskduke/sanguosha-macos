#pragma once
#include <QString>
#include <memory>
namespace qs {
class AudioBackend {
public:
 explicit AudioBackend(bool noDevice=false);
 ~AudioBackend();
 bool ready() const;
 QString error() const;
 void play(const QString &path,float volume);
 void playMusic(const QString &path,float volume);
 void setMusicVolume(float volume);
 void stopMusic();
 void stop();
 void shutdown();
 void waitForDecoders();
 int voiceCount() const;
 bool musicPlaying() const;
 // Manual mixing for the no-device integration test; never enabled in the game.
 bool render(unsigned frames);
private:
 struct Impl; std::unique_ptr<Impl> d;
 void request(const QString &,float,bool);
};
}
