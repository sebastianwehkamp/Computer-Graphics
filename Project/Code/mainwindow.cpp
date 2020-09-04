#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :  QMainWindow(parent), ui(new Ui::MainWindow) {
  qDebug() << "✓✓ MainWindow constructor";
  ui->setupUi(this);
}

MainWindow::~MainWindow() {
  qDebug() << "✗✗ MainWindow destructor";
  delete ui;

  Meshes.clear();
  Meshes.squeeze();

  LimitMeshes.clear();
  LimitMeshes.squeeze();
}

void MainWindow::importOBJ() {
  OBJFile newModel = OBJFile(QFileDialog::getOpenFileName(this, "Import OBJ File", "models/", tr("Obj Files (*.obj)")));
  ui->MainDisplay->originalMesh = new Mesh(&newModel);
  ui->MainDisplay->currentMesh = ui->MainDisplay->originalMesh;

  ui->MainDisplay->limitMesh = new Mesh();
  generateLimitMesh(ui->MainDisplay->currentMesh, ui->MainDisplay->limitMesh);

  ui->MainDisplay->updateMeshBuffers();
  ui->MainDisplay->modelLoaded = true;

  ui->MainDisplay->update();
}

void MainWindow::on_ImportOBJ_clicked() {
  importOBJ();
  ui->ImportOBJ->setEnabled(false);
  ui->SubdivSteps->setEnabled(true);
}

void MainWindow::on_RotationDial_valueChanged(int value) {
  ui->MainDisplay->rotAngle = value;
  ui->MainDisplay->updateMatrices();
}

void MainWindow::on_SubdivSteps_valueChanged(int value) {
  if (value == 0) {
    ui->MainDisplay->currentMesh = ui->MainDisplay->originalMesh;
  } else if (value >= currentMeshIndex && currentMeshIndex > 0) {
    for (int i = currentMeshIndex; i < value; i++) {
      Mesh* newMesh = new Mesh();
      subdivideCatmullClark(ui->MainDisplay->currentMesh, newMesh);
      delete ui->MainDisplay->currentMesh;
      ui->MainDisplay->currentMesh = newMesh;
    }
  } else {
    ui->MainDisplay->currentMesh = ui->MainDisplay->originalMesh;
    Mesh* newMesh = new Mesh();
    subdivideCatmullClark(ui->MainDisplay->currentMesh, newMesh);
    ui->MainDisplay->currentMesh = newMesh;

    for (int i = 1; i < value; i++) {
      Mesh* newMesh = new Mesh();
      subdivideCatmullClark(ui->MainDisplay->currentMesh, newMesh);
      delete ui->MainDisplay->currentMesh;
      ui->MainDisplay->currentMesh = newMesh;
    }
  }

  currentMeshIndex = value;

  delete ui->MainDisplay->limitMesh;
  ui->MainDisplay->limitMesh = new Mesh();
  generateLimitMesh(ui->MainDisplay->currentMesh, ui->MainDisplay->limitMesh);

  ui->MainDisplay->updateMeshBuffers();
}

void MainWindow::on_limitModeCheckbox_toggled(bool checked)
{
  ui->MainDisplay->limitMode = checked;
  ui->MainDisplay->updateMeshBuffers();
}

void MainWindow::on_showControlCheckbox_toggled(bool checked)
{
    ui->MainDisplay->showControlNet = checked;
    ui->MainDisplay->updateMatrices();
}

void MainWindow::on_sharpnessBox_valueChanged(double value)
{
    ui->MainDisplay->sharpness = value;
    ui->MainDisplay->updateSharpness();

    if (currentMeshIndex > 0) {
      ui->MainDisplay->currentMesh = ui->MainDisplay->originalMesh;
      Mesh* newMesh = new Mesh();
      subdivideCatmullClark(ui->MainDisplay->currentMesh, newMesh);
      ui->MainDisplay->currentMesh = newMesh;

      for (int i = 1; i < currentMeshIndex; i++) {
        Mesh* newMesh = new Mesh();
        subdivideCatmullClark(ui->MainDisplay->currentMesh, newMesh);
        delete ui->MainDisplay->currentMesh;
        ui->MainDisplay->currentMesh = newMesh;
      }
      ui->MainDisplay->updateMeshBuffers();
    }
}
