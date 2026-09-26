#include "paths.h"
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
namespace qs {
QString dataRoot(){
 QString root=qEnvironmentVariable("QS_DATA_DIR");
 if(root.isEmpty())root=QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+"/QSanguoshaNirvana";
 return QDir(root).absolutePath();
}
QString userPath(const QString &relative){
 const QString clean=QDir::cleanPath(relative);
 if(QDir::isAbsolutePath(clean)||clean==".."||clean.startsWith("../"))throw std::invalid_argument("userPath requires a confined relative path");
 const QString result=QDir(dataRoot()).filePath(clean);
 if(!QDir().mkpath(QFileInfo(result).absolutePath()))throw std::runtime_error("Cannot create application data directory");
 return result;
}
QString configFile(){return userPath("config.ini");}
QString readOverride(const QString &relative){if(QDir::isAbsolutePath(relative))return relative;const QString path=QDir(dataRoot()).filePath(relative);return QFileInfo::exists(path)?path:relative;}
}
