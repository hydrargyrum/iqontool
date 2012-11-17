#include "icnswriter.h"
#include <QIODevice>
#include <QIcon>
#include <QByteArray>
#include <QtEndian>
#include <QtDebug>

/** Packbits algorithm */
class Packer {
public:
	QByteArray packBuffer(const QByteArray &input) {
		QByteArray ret;

		int start = 0;
		while (start < input.length()) {
			start += createBlock(input, start, &ret);
		}

		return ret;
	}

private:
	int createBlock(const QByteArray &input, int start, QByteArray *ret) {
		if (!hasCompressableStart(input, start))
			return createUncompressedBlock(input, start, ret);
		else
			return createCompressedBlock(input, start, ret);
	}

	inline bool hasCompressableStart(const QByteArray &input, int start) {
		int sizeLeft = input.size() - start;
		return (sizeLeft >= 3 &&
		        input.at(start) == input.at(start + 1) &&
		        input.at(start) == input.at(start + 2));
	}

	int createCompressedBlock(const QByteArray &input, int start, QByteArray *ret) {
		char rep = input.at(start);
		int i;
		int processed;
		for (i = start + 1, processed = 1; i < input.size() && processed < 130; i++) {
			if (input.at(i) != rep)
				break;
			processed++;
		}

		//ret->append(-char(processed - 1)); // many webpages are wrong on this (and wikipedia)
		ret->append(char(processed + 125));
		ret->append(rep);
		return processed;
	}

	int createUncompressedBlock(const QByteArray &input, int start, QByteArray *ret) {
		int i;
		int processed;
		for (i = start + 1, processed = 1; i < input.size() && processed < 128; i++) {
			if (hasCompressableStart(input, i))
				break;
			processed++;
		}

		ret->append(char(processed - 1));
		ret->append(input.mid(start, processed));
		return processed;
	}
};

template <typename T>
static QByteArray makeBE(T number) {
	QByteArray bytes(sizeof(T), 0);
	qToBigEndian(number, (uchar*)bytes.data());
	return bytes;
}

IcnsWriter::IcnsWriter() {
}

void IcnsWriter::writeIcon(QIODevice *out, const QIcon &icon) {
	IcnsWriter writer;
	
	/* icns format: "icns"[FILE_LENGTH][ICONHEADER][ICONDATA][ICONHEADER][ICONDATA]... */
	
	qint64 fileStart = out->pos();
	out->write("icns");
	qint64 placeholderPos = out->pos();
	out->write(makeBE(qint32(0))); // placeholder

	foreach (QSize size, icon.availableSizes()) {
		writer.writeImage(out, icon.pixmap(size));
	}

	qint64 len = out->pos() - fileStart;
	out->seek(placeholderPos);
	out->write(makeBE(quint32(len)));
}

void IcnsWriter::writeImage(QIODevice *out, const QPixmap &pix) {
	/* iconheader format: char[4] magictype, int icondata_size */
	/* icondata format: depends on magictype. there are a lot of different formats.
	   roughly: small sizes = basic format optionnaly compressed with "packbits"
	            alpha channel = separate iconheader and separate icondata
	            bigger sizes = PNG or JPEG2000 (alpha not separated) */
	
	if (iconType(pix) == "FAIL")
		return;

	if (pix.width() >= 256) {
		writeBigImage(out, pix);
	} else {
		writeRGB(out, pix);
		//if (pix.hasAlpha())
		writeAlpha(out, pix); // icns are considered transparent if no alpha layer, WTF...
	}
}

void IcnsWriter::writeBigImage(QIODevice *out, const QPixmap &pix) {
	// iconheader
	out->write(iconType(pix));

	qint64 placePos = out->pos();
	out->write(makeBE(qint32(0))); // placeholder

	// icondata
	pix.save(out, "PNG");
	qint64 endPos = out->pos();

	// fix header
	qint64 dataLen = endPos - placePos + 4;

	out->seek(placePos);
	out->write(makeBE(quint32(dataLen)));
	out->seek(endPos);
}

void IcnsWriter::writeRGB(QIODevice *out, const QPixmap &pix) {
	// iconheader
	out->write(iconType(pix));

	qint64 placePos = out->pos();
	out->write(makeBE(qint32(0))); // placeholder

	bool usePackbits = true; // also works if disabled if we want too

	if (pix.width() >= 128) {
		usePackbits = true;
		out->write(QByteArray(4, 0)); // yet another WTF...
	}

	// icondata
	QImage image = pix.toImage();
	if (!usePackbits) {
		// [0RGB][0RGB][0RGB]... line by line
		for (int line = 0; line < image.height(); line++) {
			QRgb *pixel = (QRgb*) image.constScanLine(line);
			QByteArray buf;
			for (int col = 0; col < image.width(); col++) {
				// we write a 24bits image, but icns needs a we need zeros, WTF...
				buf.append('\0');
				buf.append(qint8(qRed(*pixel)));
				buf.append(qint8(qGreen(*pixel)));
				buf.append(qint8(qBlue(*pixel)));
				pixel++;
			}
			out->write(buf);
		}
	} else {
		// packbits(all red)+packbits(all green)+packbits(all blue)
		// needless to say, icns is an ugly format
		QByteArray buf;
		for (int line = 0; line < image.height(); line++) {
			QRgb *pixel = (QRgb*) image.constScanLine(line);
			for (int col = 0; col < image.width(); col++) {
				buf.append(qint8(qRed(*pixel)));
				pixel++;
			}
		}
		out->write(Packer().packBuffer(buf));
		buf.clear();
		for (int line = 0; line < image.height(); line++) {
			QRgb *pixel = (QRgb*) image.constScanLine(line);
			for (int col = 0; col < image.width(); col++) {
				buf.append(qint8(qGreen(*pixel)));
				pixel++;
			}
		}
		out->write(Packer().packBuffer(buf));
		buf.clear();
		for (int line = 0; line < image.height(); line++) {
			QRgb *pixel = (QRgb*) image.constScanLine(line);
			for (int col = 0; col < image.width(); col++) {
				buf.append(qint8(qBlue(*pixel)));
				pixel++;
			}
		}
		out->write(Packer().packBuffer(buf));
	}

	qint64 endPos = out->pos();

	// fix iconheader
	qint64 dataLen = endPos - placePos + 4;

	out->seek(placePos);
	out->write(makeBE(quint32(dataLen)));
	out->seek(endPos);
}

void IcnsWriter::writeAlpha(QIODevice *out, const QPixmap &pix) {
	// iconheader
	out->write(iconType(pix, true));

	qint64 placePos = out->pos();
	out->write(makeBE(qint32(0))); // placeholder

	// icondata
	QImage image = pix.toImage();
	for (int line = 0; line < image.height(); line++) {
		QRgb *pixel = (QRgb*) image.constScanLine(line);
		QByteArray buf;
		for (int col = 0; col < image.width(); col++) {
			buf.append(qint8(qAlpha(*pixel)));
			pixel++;
		}
		out->write(buf);
	}

	qint64 endPos = out->pos();

	// fix header
	qint64 dataLen = endPos - placePos + 4;

	out->seek(placePos);
	out->write(makeBE(quint32(dataLen)));
	out->seek(endPos);
}

QByteArray IcnsWriter::iconType(const QPixmap &pix, bool isAlpha) {
	switch (pix.width()) {
	case 16:
		if (isAlpha)
			return "s8mk";
		else
			return "is32";
	case 32:
		if (isAlpha)
			return "l8mk";
		else
			return "il32";
	case 48:
		if (isAlpha)
			return "h8mk";
		else
			return "ih32";
	case 128:
		if (isAlpha)
			return "t8mk";
		else
			return "it32";
	case 256:
		return "ic08";
	case 512:
		return "ic09";
	case 1024:
		return "ic10";
	default:
		return "FAIL"; // our custom value for error
	}
}
