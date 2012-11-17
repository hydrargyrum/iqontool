#include <QtCore>
#include <QtGui>
#include <QtDebug>
#include "icnswriter.h"
#include "icowriter.h"

int main(int argc, char *argv[]) {
	QApplication a(argc, argv);

	QStringList images = a.arguments();
	qDebug() << images;
	if (images.size() < 3) {
		qDebug() << "Usage: iqontool IMAGE [IMAGE...] OUTPUT.{ico,icns}";
		return 1;
	}
	
	images.takeFirst(); // program name
	QString outputName = images.takeLast();

	QIcon allImages;
	foreach (QString image, images) {
		allImages.addFile(image);
	}

	qDebug() << allImages.availableSizes();

	QFile output(outputName);
	output.open(QIODevice::WriteOnly | QIODevice::Truncate);
	if (output.fileName().endsWith(".icns")) {
		IcnsWriter writer;
		writer.writeIcon(&output, allImages);
	} else {
		IcoWriter writer;
		writer.writeIcon(&output, allImages);
	}
	output.close();

    return 0;
}
