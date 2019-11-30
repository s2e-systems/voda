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

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>

#include "videoddspublisher.h"

int main(int argc, char *argv[])
{
	QApplication application(argc, argv);
	application.setApplicationName("Video DDS Publisher");
	qDebug() << "This is" << application.applicationName();

	QCommandLineParser parser;
	parser.setApplicationDescription("VideoDDSpublisher");
	parser.addHelpOption();
	parser.addVersionOption();

	QCommandLineOption useTestSrcOption("testsrc", "Use test src, instead of autovideosrc.");
	parser.addOption(useTestSrcOption);

	QCommandLineOption useOmxOption("omx", "Use omx as the encoder.");
	parser.addOption(useOmxOption);

	QCommandLineOption useFixedCapsOption("fixed", "Use fixed capabilities for the camera source.");
	parser.addOption(useFixedCapsOption);

	QCommandLineOption strengthOption("strength", "Set DDS OwnershipStrength (The higher the number the more stregnth).", "[0 1500]", "1000");
	parser.addOption(strengthOption);

	parser.process(application);

	VideoDDSpublisher pub{parser.isSet(useTestSrcOption), parser.isSet(useOmxOption), parser.isSet(useFixedCapsOption), parser.value(strengthOption).toInt()};

	return application.exec();
}
