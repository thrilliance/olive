/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#ifndef VIEWER_WIDGET_H
#define VIEWER_WIDGET_H

#include <QLabel>
#include <QPushButton>
#include <QScrollBar>
#include <QTimer>
#include <QWidget>

#include "common/rational.h"
#include "viewerglwidget.h"
#include "widget/playbackcontrols/playbackcontrols.h"
#include "widget/timeruler/timeruler.h"

/**
 * @brief An OpenGL-based viewer widget with playback controls (a PlaybackControls widget).
 */
class ViewerWidget : public QWidget
{
  Q_OBJECT
public:
  ViewerWidget(QWidget* parent);

  void SetPlaybackControlsEnabled(bool enabled);

  void SetTimeRulerEnabled(bool enabled);

  void SetTimebase(const rational& r);

  const double& scale();

  rational GetTime();

  void SetScale(const double& scale_);

  void SetTime(const int64_t& time);

  void TogglePlayPause();

  bool IsPlaying();

public slots:
  /**
   * @brief Set the texture to draw and draw it
   *
   * Wrapper function for ViewerGLWidget::SetTexture().
   *
   * @param tex
   */
  void SetTexture(GLuint tex);

  void GoToStart();

  void PrevFrame();

  void Play();

  void Pause();

  void NextFrame();

  void GoToEnd();

signals:
  void TimeChanged(const rational&);

protected:
  virtual void resizeEvent(QResizeEvent *event) override;

private:
  void UpdateTimeInternal(int64_t i);

  ViewerGLWidget* gl_widget_;

  PlaybackControls* controls_;

  TimeRuler* ruler_;

  QScrollBar* scrollbar_;

  rational time_base_;

  double time_base_dbl_;

  QTimer playback_timer_;

  qint64 start_msec_;
  int64_t start_timestamp_;

private slots:
  void RulerTimeChange(int64_t);

  void PlaybackTimerUpdate();

};

#endif // VIEWER_WIDGET_H
