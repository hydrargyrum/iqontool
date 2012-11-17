#ifndef ICNSWRITER_H
#define ICNSWRITER_H

class QIODevice;
class QIcon;
class QPixmap;
class QByteArray;

class IcnsWriter {
public:
    IcnsWriter();

	/** Write all images from `icon` to `output` device in .icns format. `output` must be seekable */
	static void writeIcon(QIODevice *output, const QIcon &icon);

private:
	void writeImage(QIODevice *out, const QPixmap &pix);

	void writeBigImage(QIODevice *out, const QPixmap &pix);

	void writeRGB(QIODevice *out, const QPixmap &pix);
	void writeAlpha(QIODevice *out, const QPixmap &pix);

	QByteArray iconType(const QPixmap &pix, bool isAlpha = false);
};

#endif // ICNSWRITER_H
