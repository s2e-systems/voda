// Copyright 2017 S2E Software, Systems and Control 
//  
// Licensed under the Apache License, Version 2.0 the "License"; 
// you may not use this file except in compliance with the License. 
// You may obtain a copy of the License at 
//  
//    http://www.apache.org/licenses/LICENSE-2.0 
//  
// Unless required by applicable law or agreed to in writing, software 
// distributed under the License is distributed on an "AS IS" BASIS, 
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
// See the License for the specific language governing permissions and 
// limitations under the License.

#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>

//Forward declarations
class QOpenGLShaderProgram;
class QOpenGLDebugLogger;
//class QOpenGLFunctions;

class VideoWidget : public QOpenGLWidget, public QOpenGLFunctions_3_3_Core
{
public:
	VideoWidget(QWidget* parent = nullptr, Qt::WindowFlags windowFlags = Qt::WindowFlags());
	void openGLdebug();

	void bindTexture();

	QSize aspectRatio() const;
	void setAspectRatio(const QSize& aspectRatio);

	virtual QSize sizeHint() const;

	QRect vpRect() const;

protected:

	void initializeGL();
	void paintGL();
	void resizeGL(int width, int height);
	virtual void mouseDoubleClickEvent(QMouseEvent* event);

private:
	QOpenGLDebugLogger* m_glLogger;
	QOpenGLShaderProgram* m_shader;
	GLuint m_vertexArrayObject;
	GLuint m_textureName;

	QSize m_aspectRatio;
	QRect m_vpRect;
};

#endif // VIDEOWIDGET_H
