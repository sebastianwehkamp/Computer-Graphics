#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "objfile.h"
#include <QFileDialog>
#include "mesh.h"
#include "meshtools.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {

  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

  void importOBJ();
  Ui::MainWindow *ui;
private:
  unsigned int currentMesh = 0;

private slots:
  void on_ImportOBJ_clicked();
  void on_RotationDial_valueChanged(int value);
  void on_SubdivSteps_valueChanged(int value);
  void on_limitPoints_toggled(bool checked);
  void on_tesselation_toggled(bool checked);

private:
};

#endif // MAINWINDOW_H
