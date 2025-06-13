#include "SoundManager.h"
#include <QDebug>
#include <QMediaPlaylist>

SoundManager& SoundManager::instance()
{
    static SoundManager instance;
    return instance;
}

SoundManager::SoundManager(QObject* parent)
    : QObject(parent)
    , m_Player(nullptr)
    , m_volume(50) // 默认音量50%
{
    initializeSounds();
}

SoundManager::~SoundManager()
{
    stopBGM();
    qDeleteAll(m_soundEffects);
    delete m_Player;
}

void SoundManager::initializeSounds()
{
    // 初始化BGM播放器
    m_Player = new QMediaPlayer(this);
    QMediaPlaylist* playlist = new QMediaPlaylist(this);

    playlist->addMedia(QUrl("qrc:/mus/res/BGM_1.wav"));
    playlist->addMedia(QUrl("qrc:/mus/res/BGM_2.wav"));
    playlist->addMedia(QUrl("qrc:/mus/res/BGM_3.wav"));

    playlist->setPlaybackMode(QMediaPlaylist::Loop);
    m_Player->setPlaylist(playlist);

    // 定义音效文件列表
    QMap<QString, QString> soundFiles;
    soundFiles["card_play"] = "qrc:/mus/res/card_play.mp3";
    soundFiles["card_deal"] = "qrc:/mus/res/card_deal.mp3";

    // 清理并重新加载音效
    qDeleteAll(m_soundEffects);
    m_soundEffects.clear();

	// 遍历音效文件列表，设置音频文件
    for (auto it = soundFiles.constBegin(); it != soundFiles.constEnd(); ++it) {
        // 创建 QMediaPlayer 对象
        QMediaPlayer* effectPlayer = new QMediaPlayer(this);
        effectPlayer->setMedia(QUrl(it.value()));
        m_soundEffects[it.key()] = effectPlayer;
    }

    updateVolume();
}

void SoundManager::setVolume(int volume)
{
    m_volume = qBound(0, volume, 100);
    updateVolume();
}

int SoundManager::getVolume() const
{
    return m_volume;
}

void SoundManager::updateVolume()
{
    float volume = m_volume / 100.0f;
    m_Player->setVolume(qRound(volume * 100));

    for (QMediaPlayer* effectPlayer : m_soundEffects) {
        effectPlayer->setVolume(m_volume);
    }
}

void SoundManager::playBGM()
{
    if (m_Player->state() != QMediaPlayer::PlayingState) {
        m_Player->play();
    }
}

void SoundManager::stopBGM()
{
    m_Player->stop();
}

void SoundManager::playCardPlaySound()
{
    if (m_soundEffects.contains("card_play")) {
        QMediaPlayer* player = m_soundEffects["card_play"];
        // 如果正在播放，先停止并回到开头，再播放，以实现快速重复触发
        if (player->state() == QMediaPlayer::PlayingState) {
            player->stop();
        }
        player->play();
    }
}

void SoundManager::playButtonClickSound()
{
    if (m_soundEffects.contains("card_deal")) {
        QMediaPlayer* player = m_soundEffects["card_deal"];
        if (player->state() == QMediaPlayer::PlayingState) {
            player->stop();
        }
        player->play();
    }
}