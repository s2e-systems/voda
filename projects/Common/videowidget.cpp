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

#include "videowidget.h"

#include <QOpenGLDebugLogger>
#include <QOpenGLShaderProgram>
#include <QDebug>
#include <QEvent>
#include <QPainter>

VideoWidget::VideoWidget(QWidget* parent, Qt::WindowFlags windowFlags) :
	QOpenGLWidget(parent, windowFlags)
	,m_glLogger(nullptr)
	,m_shader(nullptr)
	,m_aspectRatio(640, 480)
{
	QSurfaceFormat newformat = format();
//	newformat.setMajorVersion(3);
//	newformat.setMinorVersion(3);
	newformat.setProfile(QSurfaceFormat::CoreProfile);
	newformat.setOption(QSurfaceFormat::DebugContext);
	setFormat(newformat);
}

void VideoWidget::initializeGL()
{
	QString const vertexShaderStr = QString(
		"#version 330 core						\n"
		"// Input from the application			\n"
		"in vec2 inTexCoord;					\n"
		"in vec4 inVertexLoc;					\n"
		"										\n"
		"// Output for the fragment shader		\n"
		"varying vec2 vsTexCoords;				\n"
		"										\n"
		"void main()							\n"
		"{										\n"
		"	gl_Position = inVertexLoc;			\n"
		"	vsTexCoords = inTexCoord;			\n"
		"}										\n"
	);

	QString const fragmentShaderStr = QString(
		"#version 330 core											\n"
		"//Input from vertex shader:								\n"
		"varying vec2 vsTexCoords;									\n"
		"//From application:										\n"
		"uniform sampler2D textureSampler;							\n"
		"															\n"
		"void main()												\n"
		"{															\n"
		"	gl_FragColor = texture2D(textureSampler, vsTexCoords);	\n"
		"}															\n"
	);

	bool ret;

	ret = initializeOpenGLFunctions();
	if (ret == false)
	{
		qCritical() << "Could not initialize GL functions";
		return;
	}

	m_glLogger = new QOpenGLDebugLogger(this);
	m_glLogger->initialize(); // initializes in the current context

	m_shader = new QOpenGLShaderProgram(this);

	ret = m_shader->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderStr);
	if (ret == false)
	{
		qCritical() << "Could not load vertex shader";
	}
	ret = m_shader->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderStr);
	if (ret == false)
	{
		qCritical() << "Could not load fragment shader";
	}

	ret = m_shader->link();
	if (ret == false)
	{
		qCritical() << "Could not link the shaders";
	}

	ret = m_shader->bind();
	if (ret == false)
	{
		qCritical() << "Could not bind background shader";
	}

	GLuint vertexLocation = m_shader->attributeLocation("inVertexLoc");
	if (vertexLocation == -1)
	{
		qCritical() << "vertexLocation could not be found.";
	}
	GLuint texCoordLocation = m_shader->attributeLocation("inTexCoord");
	if (texCoordLocation == -1)
	{
		qCritical() << "texCoordLocation could not be found.";
	}

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0);


	// Vertex data for the GPU
	static const GLfloat bufferData[] =
	{
		//Vertex locations
		-1.0f, -1.0f,
		1.0f, -1.0f,
		1.0f, 1.0f,
		-1.0f, 1.0f,
		// Tex coords
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f
	};

	// VBO - Vertex Buffer Object
	GLuint vertexBufferObject;
	glGenBuffers(1, &vertexBufferObject);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
	//Allocate space to the vertex buffer object as it is currently bound to GL_ARRAY_BUFFER
	glBufferData(GL_ARRAY_BUFFER, sizeof(bufferData), bufferData, GL_STATIC_DRAW);

	// VAO - Vertex Array Object
	glGenVertexArrays(1, &m_vertexArrayObject);
	glBindVertexArray(m_vertexArrayObject);

	// Define the memory location that is associated with the shader locations
	// Note: this must be done after the Vertex Buffer Object is bound
	glVertexAttribPointer(vertexLocation, 2 /*nrOfComponents*/, GL_FLOAT /*type*/,
						  GL_FALSE /*normalized*/, 0 /*stride*/,
						  reinterpret_cast<const void*>(0) /*pointer*/);
	glVertexAttribPointer(texCoordLocation, 2 /*nrOfComponents*/, GL_FLOAT /*type*/,
						  GL_FALSE /*normalized*/, 0 /*stride*/,
						  reinterpret_cast<const void*>(8 * sizeof(float)) /*pointer*/);
	// Enable shader locations
	glEnableVertexAttribArray(vertexLocation);
	glEnableVertexAttribArray(texCoordLocation);

	// Textures
	glGenTextures(1, &m_textureName);

	glBindTexture(GL_TEXTURE_2D, m_textureName);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

void VideoWidget::paintGL()
{
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_CULL_FACE);

	bindTexture();

	// Qt resets the viewport to rect() everytime paintGL() is called.
	// Hence set it everytime to the viewport rectangle
	glViewport(m_vpRect.left(), m_vpRect.top(), m_vpRect.width(), m_vpRect.height());
	glDrawArrays(GL_TRIANGLE_FAN, 0 /*first*/, 4 /*count*/);
}

void VideoWidget::resizeGL(int width, int height)
{
	m_vpRect = QRect(QPoint(0,0), m_aspectRatio.scaled(width, height, Qt::KeepAspectRatio));
	m_vpRect.moveCenter(rect().center());
}

void VideoWidget::mouseDoubleClickEvent(QMouseEvent* )
{
	if (isFullScreen() == true)
	{
		showNormal();
	}
	else
	{
		showFullScreen();
	}
}

QRect VideoWidget::vpRect() const
{
	return m_vpRect;
}

QSize VideoWidget::aspectRatio() const
{
	return m_aspectRatio;
}

void VideoWidget::setAspectRatio(const QSize& aspectRatio)
{
	m_aspectRatio = aspectRatio;
}

QSize VideoWidget::sizeHint() const
{
	return m_aspectRatio;
}

void VideoWidget::bindTexture()
{
	glBindTexture(GL_TEXTURE_2D, m_textureName);
}

void VideoWidget::openGLdebug()
{
	if (m_glLogger != nullptr)
	{
		QList<QOpenGLDebugMessage> messages = m_glLogger->loggedMessages();
		foreach (const QOpenGLDebugMessage &message, messages)
		{
			qDebug() << message;
		}
	}
}
