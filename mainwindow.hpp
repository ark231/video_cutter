#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAudioOutput>
#include <QMainWindow>
#include <QMediaPlayer>
#include <chrono>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

   private slots:
    void play_pause_button_pressed();

    void change_video_position(std::chrono::seconds newposition);
    void update_video_position_display(std::chrono::seconds newposition);
    void update_video_position_slider(std::chrono::seconds newposition);

    void update_start_time_display(std::chrono::seconds time);
    void update_start_time_slider(std::chrono::seconds time);

    void update_end_time_display(std::chrono::seconds time);
    void update_end_time_slider(std::chrono::seconds time);

    void open_video();

    void update_duration(int newduration);

    void show_videoerror(QMediaPlayer::Error, const QString &error_string);

    void save_result();

    void change_curent_time_slider_tracking_state(bool state);

    void change_time_appending_is_enabled(bool state);

   private:
    Ui::MainWindow *ui;  // NOLINT(readability-identifier-naming)
    QMediaPlayer player_;
    QAudioOutput audio_output_;
    bool time_appending_is_enabled_ = true;
};
#endif  // MAINWINDOW_H
