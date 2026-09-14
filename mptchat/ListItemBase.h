//
// Created by mpt on 2026/7/12.
//

#ifndef MPTCHAT_LISTITEMBASE_H
#define MPTCHAT_LISTITEMBASE_H
#include <QObject>
#include "global.h"

class ListItemBase : public QWidget{
    Q_OBJECT
public:
    ListItemBase(QWidget *parent = nullptr);

public:
    void SetItemType(ListItemType itemType);

    void paintEvent(QPaintEvent *event) override;

    ListItemType GetItemType();

private:
    ListItemType _itemType;
};


#endif //MPTCHAT_LISTITEMBASE_H