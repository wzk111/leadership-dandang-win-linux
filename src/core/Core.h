#pragma once
#include <QString>
#include <QList>
#include <QStringList>
#include <QSettings>
#include <optional>
#include <QRect>

namespace ws {
struct Selection { QString text; std::optional<QRect> anchorRect; QString sourceApplication; };
enum class Feature { PlainSpeak, Summarize, Polish, Reply, Relevance, AddInsight };
struct FeatureInfo { Feature id; QString name; QString description; bool requiresProfile; QString stableId; QString shortName; bool requiresComposer=false; QString iconName; QStringList supportedVariants; };
class FeatureRegistry {
public:
    static QList<FeatureInfo> all();
    static FeatureInfo info(Feature);
    static std::optional<Feature> fromId(const QString&);
};
struct PromptRequest { Feature feature; QString selectedText; QString outputLanguage; QString profile; QString variant,userIntent,refinementInstruction,previousResult; };
struct Prompt { QString system; QString user; QString error; };
class PromptBuilder {
public:
    static Prompt build(const PromptRequest& request);
};
struct Settings {
    QString model;
    QString outputLanguage = QStringLiteral("Same as input");
    QString providerId="openai"; QString baseUrl; int maxOutputTokens=2048;
    static Settings load(QSettings& store);
    static Settings forProvider(QSettings&,const QString&);
    bool save(QSettings& store) const;
};
}
