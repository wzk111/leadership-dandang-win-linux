#pragma once
#include <QString>
#include <QList>
#include <QSettings>
#include <optional>
#include <QRect>

namespace ws {
struct Selection { QString text; std::optional<QRect> anchorRect; QString sourceApplication; };
enum class Feature { PlainSpeak, Summarize, Polish };
struct FeatureInfo { Feature id; QString name; QString description; bool requiresProfile; };
class FeatureRegistry {
public:
    static QList<FeatureInfo> all();
};
struct PromptRequest { Feature feature; QString selectedText; QString outputLanguage; QString profile; };
struct Prompt { QString system; QString user; QString error; };
class PromptBuilder {
public:
    static Prompt build(const PromptRequest& request);
};
struct Settings {
    QString model;
    QString outputLanguage = QStringLiteral("Same as input");
    static Settings load(QSettings& store);
    bool save(QSettings& store) const;
};
}
