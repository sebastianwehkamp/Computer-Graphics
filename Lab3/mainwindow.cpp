#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :  QMainWindow(parent), ui(new Ui::MainWindow) {
  qDebug() << "✓✓ MainWindow constructor";
  ui->setupUi(this);
}

MainWindow::~MainWindow() {
  qDebug() << "✗✗ MainWindow destructor";
  ui->MainDisplay->Meshes.clear();
  ui->MainDisplay->Meshes.squeeze();

  delete ui;
}


void MainWindow::importOBJ() {
  OBJFile newModel = OBJFile(QFileDialog::getOpenFileName(this, "Import OBJ File", "models/", tr("Obj Files (*.obj)")));
  ui->MainDisplay->Meshes.clear();
  ui->MainDisplay->Meshes.squeeze();
  ui->MainDisplay->Meshes.append( Mesh(&newModel) );

  ui->ImportOBJ->setEnabled(false);

  ui->MainDisplay->updateMeshBuffers( &ui->MainDisplay->Meshes[0] );
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
  unsigned short k;

  for (k=ui->MainDisplay->Meshes.size(); k<value+1; k++) {
    ui->MainDisplay->Meshes.append(Mesh());
    subdivideCatmullClark(&ui->MainDisplay->Meshes[k-1], &ui->MainDisplay->Meshes[k]);
  }
  currentMesh = value;
  ui->MainDisplay->currentMeshIndex = value;

  ui->MainDisplay->updateMeshBuffers( &ui->MainDisplay->Meshes[value] );
  ui->MainDisplay->buildQuadMesh();
  ui->limitPoints->setChecked(false);
}


// Added buttons for tesselation and limit computation
void MainWindow::on_limitPoints_toggled(bool checked){
    ui->MainDisplay->showLimit = checked;
    qDebug() << "Limit points checked";

    if (checked){
        qDebug() << "Checked";
        ui->MainDisplay->limitMesh = new Mesh();
        qDebug() << "Running tolimit";
        computeLimitMesh(&ui->MainDisplay->Meshes[currentMesh], ui->MainDisplay->limitMesh);
        qDebug() << "Update mesh buffers";
        ui->MainDisplay->updateMeshBuffers( ui->MainDisplay->limitMesh );
        qDebug() << "build quad mesh";
        ui->MainDisplay->buildQuadMesh();
    } else {
        ui->MainDisplay->updateMeshBuffers( &ui->MainDisplay->Meshes[currentMesh] );
        ui->MainDisplay->buildQuadMesh();
    }
}

void MainWindow::on_tesselation_toggled(bool checked){
    ui->MainDisplay->showTesselation = checked;
    ui->MainDisplay->updateMeshBuffers( &ui->MainDisplay->Meshes[currentMesh] );
    ui->MainDisplay->buildQuadMesh();
    ui->MainDisplay->updateMatrices();
}
