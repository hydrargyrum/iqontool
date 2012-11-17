#ifndef ICOWRITER_H
#define ICOWRITER_H

#include <QList>
class QIODevice;
class QIcon;
class QPixmap;
class QByteArray;

class IcoWriter {
public:
    IcoWriter();

	/** Write all images from `icon` to `output` in .ico format. `output` must be seekable */
	static void writeIcon(QIODevice *output, const QIcon &icon);

private:
	void writeIcoHeader(QIODevice *out, const QIcon &icon);
	void writeDirEntry(QIODevice *out, const QPixmap &pix);
	void writeImage(QIODevice *out, const QPixmap &pix);

	QList<qint64> modPositions;
};

#endif // ICOWRITER_H
