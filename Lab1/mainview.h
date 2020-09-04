#ifndef MAINVIEW_H
#define MAINVIEW_H

#include <QOpenGLFunctions_4_1_Core>
#include <QOpenGLWidget>
#include <QOpenGLDebugLogger>
#include <QOpenGLShaderProgram>

#include <QVector2D>
#include <QMouseEvent>

#include <Eigen/Dense>

class MainView : public QOpenGLWidget, protected QOpenGLFunctions_4_1_Core {

  Q_OBJECT

public:
  MainView(QWidget *parent = 0);
  ~MainView();

  void clearArrays(); // Clears net
  void clearCurve();  // Clears curve array

  void presetNet(unsigned short preset);
  void updateBuffers();

  bool showNet, showCurvePts = false;
  bool showCurveColour; // show curvature using colour
  short int selectedPt;
  short int findClosest(float x, float y);

  void setMask(QString stringMask);
  void setSteps(int value);

  void reload(); // updateBuffers + update

protected:
  void initializeGL();
  void paintGL();

  void mousePressEvent(QMouseEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void keyPressEvent(QKeyEvent *event);

private:
  QOpenGLDebugLogger* debugLogger;

  QVector<QVector2D> netCoords, curveCoords;
  QVector<QVector3D> curveColours, netColours;

  QOpenGLShaderProgram *mainShaderProg, *curveShaderProg;
  GLuint netVAO, netCoordsBO, netColourBO,
         curveVAO, curveCoordsBO, curveColourBO;

  GLuint steps;


  void createShaderPrograms();
  void createBuffers();

  bool updateUniformsRequired;
  //GLint uni...

  void updateUniforms();

  QVector<short int> subdivMask, firstStencil, secondStencil;
  float normalizeValue;

  QVector<QVector2D> calculateSubdivision(QVector<QVector2D> points);
  Eigen::MatrixXf getMatrix(int oldPoints);

private slots:
  void onMessageLogged( QOpenGLDebugMessage Message );

};

#endif // MAINVIEW_H
