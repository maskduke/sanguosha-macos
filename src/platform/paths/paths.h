#pragma once
#include <QString>
#include <stdexcept>
namespace qs {
QString dataRoot();
QString configFile();
QString userPath(const QString &relative);
QString readOverride(const QString &relative);
}
