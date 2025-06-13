#pragma once

#include <QObject>
#include <QMediaPlayer>
#include <QSoundEffect>
#include <QMap>
#include <QVector>

class SoundManager : public QObject
{
    Q_OBJECT

public:
    static SoundManager& instance();

    // 音量控制
    void setVolume(int volume); // 0-100
    int getVolume() const;

    // 音效播放
    void playBGM();
    void stopBGM();
    void playCardPlaySound();
    void playButtonClickSound();

private:
    explicit SoundManager(QObject* parent = nullptr);
    ~SoundManager();

    // 禁止拷贝和赋值
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    void initializeSounds();
    void updateVolume();
    void playRandomBGM();

    QMediaPlayer* m_Player;
    QMap<QString, QMediaPlayer*> m_soundEffects;
    QVector<QString> m_bgmList;
    int m_volume;
};

