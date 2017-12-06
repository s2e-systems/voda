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

#include "videowidgetgles2.h"

#include <QOpenGLDebugLogger>
#include <QOpenGLShaderProgram>

/**
 * This is to provide a readable manner to define
 * vertex data fo a vertex buffer array
 */
struct VertexData
{
	QVector3D position;
	QVector2D texCoord;
};

VideoWidgetGLES2::VideoWidgetGLES2(QWidget* parent, Qt::WindowFlags windowFlags) :
	QOpenGLWidget(parent, windowFlags)
	,m_aspectRatio(640, 480)
{
	// Switch on the debug contex to enable
	// the VideoWidgetGLES2::openGLdebug() function.
	//  TODO: only enable in debug builds
	QSurfaceFormat newformat = format();
	newformat.setOption(QSurfaceFormat::DebugContext);
	setFormat(newformat);
}

QSize VideoWidgetGLES2::sizeHint() const
{
	return m_aspectRatio;
}

void VideoWidgetGLES2::drawImage(const QImage& image)
{
	// Internal format of GL_RGB is not supported by
	// raspberry pi 1 & 2
	GLint const internalFormat = GL_RGBA;
	GLenum const externalType = GL_UNSIGNED_BYTE;

	// Determine external format (that is the format of the Qimage
	// in OpenGL nomenclature)
	GLenum externalFormat;

	switch (image.format())
	{
	case QImage::Format_RGBA8888:
	case QImage::Format_RGBA8888_Premultiplied:
	case QImage::Format_ARGB32:
	case QImage::Format_ARGB32_Premultiplied:
		externalFormat = GL_RGBA;
		break;
	case QImage::Format_RGB888:
		externalFormat = GL_RGB;
		break;
	default:
		qCritical() << "Image format" << image.format() << "not supported by VideoWidgetGLES2";
	}

	int const width = image.width();
	int const height = image.height();

	auto pixels = static_cast<GLvoid const* >(image.constBits());

	glTexImage2D(GL_TEXTURE_2D,
				 0 /*level*/, internalFormat, width, height,
				 0 /*border, "this value must be zero" [OpenGL doc] */,
				 externalFormat, externalType, pixels);
}

void VideoWidgetGLES2::bindTexture()
{
	glBindTexture(GL_TEXTURE_2D, m_textureName);
}

void VideoWidgetGLES2::openGLdebug(const QString& msg)
{
	if (m_glLogger != nullptr)
	{
		qDebug() << "VideoWidgetGLES2::openGLdebug " << msg << ":";
		QList<QOpenGLDebugMessage> messages = m_glLogger->loggedMessages();
		if (messages.length() == 0)
		{
			qDebug() << "\t All Good.";
		}
		foreach (const QOpenGLDebugMessage &message, messages)
		{
			qDebug().noquote() << "\t" << message;
		}
	}
}

void VideoWidgetGLES2::initializeGL()
{
	QString const vertexShaderStr = QString(
		"attribute vec2 inTexCoord;				\n"
		"attribute vec4 inVertexLoc;			\n"
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
		"															\n"
		"//Input from vertex shader:								\n"
		"varying vec2 vsTexCoords;									\n"
		"															\n"
		"//From application:										\n"
		"uniform highp sampler2D textureSampler;					\n"
		"															\n"
		"															\n"
		"void main()												\n"
		"{															\n"
		"	gl_FragColor = texture2D(textureSampler, vsTexCoords);	\n"
		"}															\n"
	);

	bool ret;

	initializeOpenGLFunctions();

	m_arrayBuf.create();

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

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable(GL_CULL_FACE);

	VertexData vertices[] = {
		{QVector2D(-1.0f, -1.0f), QVector2D(0.0f, 1.0f)}, // v0
		{QVector2D( 1.0f, -1.0f), QVector2D(1.0f, 1.0f)}, // v1
		{QVector2D( 1.0f,  1.0f), QVector2D(1.0f, 0.0f)}, // v2
		{QVector2D(-1.0f,  1.0f), QVector2D(0.0f, 0.0f)}, // v3
	};

	// Transfer vertex data to VBO 0
	m_arrayBuf.bind();
	m_arrayBuf.allocate(vertices, 4 * sizeof(VertexData));

	// Offset for position
	quintptr offset = 0;

	int vertexLocation = m_shader->attributeLocation("inVertexLoc");
	m_shader->enableAttributeArray(vertexLocation);
	m_shader->setAttributeBuffer(vertexLocation, GL_FLOAT, offset, 2 /*tupleSize*/, sizeof(VertexData));

	// Offset for texture coordinate
	offset += sizeof(QVector3D);

	int texcoordLocation = m_shader->attributeLocation("inTexCoord");
	m_shader->enableAttributeArray(texcoordLocation);
	m_shader->setAttributeBuffer(texcoordLocation, GL_FLOAT, offset, 2 /*tupleSize*/, sizeof(VertexData));

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// Textures
	glGenTextures(1, &m_textureName);

	glBindTexture(GL_TEXTURE_2D, m_textureName);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

void VideoWidgetGLES2::paintGL()
{
	bindTexture();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Qt resets the viewport to rect() everytime paintGL() is called.
	// Hence set it everytime to the viewport rectangle
	glViewport(m_vpRect.left(), m_vpRect.top(), m_vpRect.width(), m_vpRect.height());
	glDrawArrays(GL_TRIANGLE_FAN, 0 /*first*/, 4 /*count*/);
}

void VideoWidgetGLES2::resizeGL(int width, int height)
{
	m_vpRect = QRect(QPoint(0,0), m_aspectRatio.scaled(width, height, Qt::KeepAspectRatio));
	m_vpRect.moveCenter(rect().center());
}

void VideoWidgetGLES2::mouseDoubleClickEvent(QMouseEvent* event)
{
	Q_UNUSED(event)

	if (isFullScreen() == true)
	{
		showNormal();
	}
	else
	{
		showFullScreen();
	}
}

QRect VideoWidgetGLES2::vpRect() const
{
	return m_vpRect;
}

QSize VideoWidgetGLES2::aspectRatio() const
{
	return m_aspectRatio;
}

void VideoWidgetGLES2::setAspectRatio(const QSize& aspectRatio)
{
	m_aspectRatio = aspectRatio;
}
