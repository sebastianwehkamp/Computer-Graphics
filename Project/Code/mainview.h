#ifndef MAINVIEW_H
#define MAINVIEW_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_1_Core>
#include <QOpenGLDebugLogger>

#include <QOpenGLShaderProgram>

#include <QMouseEvent>
#include "mesh.h"

class MainView : public QOpenGLWidget, protected QOpenGLFunctions_4_1_Core {

  Q_OBJECT

public:
  MainView(QWidget *Parent = 0);
  ~MainView();

  bool modelLoaded;
  bool wireframeMode;
  bool limitMode;
  bool showControlNet = true;

  Mesh* originalMesh;
  Mesh* currentMesh;
  Mesh* limitMesh;

  float FoV;
  float dispRatio;
  float rotAngle;
  float sharpness = 0.0;

  bool uniformUpdateRequired;

  int select_index = -1;

  void updateMatrices();
  void updateUniforms();
  void updateMeshBuffers();
  void updateSharpness();

protected:
  void initializeGL();
  void resizeGL(int newWidth, int newHeight);
  void paintGL();

  unsigned int maxInt;

  void renderMesh();

  void mousePressEvent(QMouseEvent* event);
  void wheelEvent(QWheelEvent* event);
  void keyPressEvent(QKeyEvent* event);

private:
  QOpenGLDebugLogger* debugLogger;

  QMatrix4x4 modelViewMatrix, projectionMatrix;
  QMatrix3x3 normalMatrix;

  // Uniforms
  GLint uniModelViewMatrix, uniProjectionMatrix, uniNormalMatrix;
  GLint controlUniModelViewMatrix, controlUniProjectionMatrix;

  // ---

  QOpenGLShaderProgram* mainShaderProg;
  QOpenGLShaderProgram* controlShaderProg;

  GLuint meshVAO, meshCoordsBO, meshNormalsBO, meshIndexBO;
  GLuint controlVAO, controlCoordsBO, controlIndexBO, controlColourBO;
  GLuint selectVAO, selectCoordsBO, selectIndexBO, selectColourBO;
  unsigned int meshIBOSize;

  // ---

  // Model vectors
  QVector<QVector3D> vertexCoords;
  QVector<QVector3D> vertexNormals;
  QVector<unsigned int> polyIndices;

  // Control net Vectors
  QVector<QVector3D> controlCoords;
  QVector<QVector3D> controlColours;
  QVector<unsigned int> controlIndices;

  // Vectors used for the edge which is selected
  QVector<QVector3D> selectCoords;
  QVector<QVector3D> selectColours;
  QVector<unsigned int> selectIndices;

  void createShaderPrograms();
  void createBuffers();

private slots:
  void onMessageLogged( QOpenGLDebugMessage Message );

};

#endif // MAINVIEW_H
