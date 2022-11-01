#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QAudioOutput>
#include <QDir>
#include <QMainWindow>
#include <QMediaPlayer>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <chrono>
#include <optional>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class ProcessWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

   private:
    void play_pause_button_pressed_();

    void update_current_pos_and_slider_(QTime time);
    void change_video_position_(std::chrono::milliseconds newposition);
    void update_video_position_display_(std::chrono::milliseconds newposition);
    void update_video_position_slider_(std::chrono::seconds newposition);

    void update_start_time_display_(std::chrono::seconds time);
    void update_start_time_slider_(std::chrono::seconds time);

    void update_end_time_display_(std::chrono::seconds time);
    void update_end_time_slider_(std::chrono::seconds time);

    QUrl read_video_dir_cache_();
    void write_video_dir_cache_(QUrl dir);
    void update_valid_period_of_cache_();

    void open_video_();
    void close_video_();

    void update_duration_(int newduration);

    void show_videoerror_(QMediaPlayer::Error, const QString &error_string);

    void save_result_();

    void change_curent_time_slider_tracking_state_(bool state);

    void select_savefile_name_plugin_();

   private:
    Ui::MainWindow *ui;  // NOLINT(readability-identifier-naming)
    QMediaPlayer player_;
    QAudioOutput audio_output_;

    ProcessWidget *process_;
    QSettings *settings_ = nullptr;
    std::optional<QString> savefile_name_plugin_ = std::nullopt;
    static constexpr auto NO_PLUGIN = "do not use any plugins";
#ifdef _WIN32
    static constexpr auto PYTHON = "py";
#else
    static constexpr auto PYTHON = "python";
#endif
    QDir savefile_name_plugins_dir_();
    QStringList search_savefile_name_plugins_();
    QStringList savefile_name_plugins_();
    int savefile_name_plugin_index_();

    void start_saving_();
    void confirm_save_filename_(QString default_save_filename);
    void cut_video_(QString save_filename);
};
#endif  // MAINWINDOW_H
