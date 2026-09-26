%module qt_values
%{
#include <QString>
#include <QStringList>
#include <QVariant>
%}
// The game's actual typemaps, not copies or test replacements.
%include "naturalvar.i"
%include "qvariant_core.i"
%inline %{
QString echoString(QString value) { return value; }
QStringList echoList(QStringList value) { return value; }
QString makeChinese() { return QString::fromUtf8("刘备：仁德"); }
%}
