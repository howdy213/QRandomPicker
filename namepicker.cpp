#include "namepicker.h"
#include <chrono>

NamePicker::NamePicker(QObject *parent) : QObject(parent) {
    auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
    m_randomEngine.seed(static_cast<unsigned int>(seed));
}

QString NamePicker::pickName() {
    if (m_nameList.isEmpty()) return {};

    if (m_mode == PickMode::Random) {
        std::uniform_int_distribution<int> dist(0, m_nameList.size() - 1);
        return m_nameList.at(dist(m_randomEngine));
    } else {
        if (static_cast<qsizetype>(m_pickedIndices.size()) >= m_nameList.size())
            m_pickedIndices.clear();

        QStringList unpicked;
        for (int i = 0; i < m_nameList.size(); ++i) {
            if (!m_pickedIndices.contains(i))
                unpicked.append(m_nameList.at(i));
        }

        if (unpicked.isEmpty()) return {};

        std::uniform_int_distribution<int> dist(0, unpicked.size() - 1);
        QString picked = unpicked.at(dist(m_randomEngine));
        m_pickedIndices.append(m_nameList.indexOf(picked));
        return picked;
    }
}
