#include <QtCore>
#include <QtGui>
#include <QtDebug>
#include <QtEndian>
#include "icowriter.h"

/** convert a number into little-endian representation */
template <typename T>
static QByteArray makeLE(T number) {
	QByteArray bytes(sizeof(T), 0);
	qToLittleEndian(number, (uchar*)bytes.data());
	return bytes;
}

IcoWriter::IcoWriter() {
}

void IcoWriter::writeIcoHeader(QIODevice *out, const QIcon &icon) {
	QByteArray header = makeLE(qint16(0)) + makeLE(qint16(1));

	quint16 icons = icon.availableSizes().size();
	header += makeLE(icons);

	out->write(header);
}

// not reentrant
void IcoWriter::writeDirEntry(QIODevice *out, const QPixmap &pix) {
	/* direntry format: char width, char height, char palette_length, short planes,
	   short bpp, int icondata_size, int icondata_offset_in_file */
	QByteArray header = makeLE(quint8(pix.width())) + makeLE(quint8(pix.height()));
	header += makeLE(qint8(0)); // palette colors, 0 == no palette
	header += makeLE(qint8(0)); // reserved, 0
	header += makeLE(qint16(0)); // planes
	header += makeLE(qint16(pix.depth())); // bpp

	modPositions.append(out->pos() + header.size());

	header += makeLE(qint32(0)); // size in bytes
	header += makeLE(qint32(0)); // start offset
	// ^ placeholder values

	out->write(header);
}

void IcoWriter::writeImage(QIODevice *out, const QPixmap &pix) {
	/* icondata can be any recognized format */
	qint64 oldPos = out->pos();
	pix.save(out, "PNG");
	qint64 newPos = out->pos();

	// replace the placeholder values
	qint64 headerPos = modPositions.takeFirst();
	out->seek(headerPos);
	out->write(makeLE(qint32(newPos - oldPos)));
	out->write(makeLE(qint32(oldPos)));

	// get back to writing
	out->seek(newPos);
}

void IcoWriter::writeIcon(QIODevice *out, const QIcon &icon) {
	IcoWriter writer;

	/* .ico format: [HEADER][DIRENTRY][DIRENTRY][DIRENTRY]...[ICONDATA][ICONDATA][ICONDATA]... */
	writer.writeIcoHeader(out, icon);

	foreach (QSize size, icon.availableSizes()) {
		writer.writeDirEntry(out, icon.pixmap(size));
	}

	foreach (QSize size, icon.availableSizes()) {
		writer.writeImage(out, icon.pixmap(size));
	}
}
