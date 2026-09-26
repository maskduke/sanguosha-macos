// Primitive QVariant API shared by game bindings and boundary tests.
class QVariant{
public:
	QVariant();
	QVariant(int);
	QVariant(const char *);
	QVariant(bool);
	int toInt() const;
	QString toString() const;
	QStringList toStringList() const;
	bool toBool() const;
};

