//
// Created by mpt on 2026/7/22.
//

// You may need to build the project (run Qt uic code generator) to get "ui_ApplyFriend.h" resolved

#include "applyfriend.h"

#include <QScrollBar>

#include "ui_ApplyFriend.h"
#include "UserMgr.h"
#include "TCPMgr.h"
ApplyFriend::ApplyFriend(QWidget *parent) : QDialog(parent), ui(new Ui::ApplyFriend) {
    ui->setupUi(this);

    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    this->setObjectName("ApplyFriend");
    this->setModal(true);
    ui->name_edit->setPlaceholderText(tr("恋恋风辰"));
    ui->label_edit->setPlaceholderText("搜索、添加标签");
    ui->back_edit->setPlaceholderText("燃烧的胸毛");

    ui->label_edit->SetMaxLength(21);
    ui->label_edit->setMaxLength(10);
    ui->label_edit->setParent(ui->grid_wid);
    ui->label_edit->move(2, 2);
    ui->label_edit->setFixedHeight(24);
    ui->label_edit->setFixedWidth(ui->grid_wid->width() - 4);
    ui->label_edit->show();
    ui->grid_wid->setFixedHeight(30);
    ui->input_tip_wid->hide();

    _tip_cur_point = QPoint(5, 5);

    _tip_data = { "同学","家人","菜鸟教程","C++ Primer","Rust 程序设计",
                             "父与子学Python","nodejs开发指南","go 语言开发指南",
                                "游戏伙伴","金融投资","微信读书","拼多多拼友" };

    connect(ui->more_label, &ClickedOnceLabel::clicked, this, &ApplyFriend::ShowMoreLabel);
    InitTipLbs();
    //链接输入标签回车事件
    connect(ui->label_edit, &CustomizeEdit::returnPressed, this, &ApplyFriend::SlotLabelEnter);
    connect(ui->label_edit, &CustomizeEdit::textChanged, this, &ApplyFriend::SlotLabelTextChange);
    connect(ui->label_edit, &CustomizeEdit::editingFinished, this, &ApplyFriend::SlotLabelEditFinished);
    connect(ui->tip_label, &ClickedOnceLabel::clicked, this, &ApplyFriend::SlotAddFirendLabelByClickTip);

    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    //连接确认和取消按钮的槽函数
    connect(ui->cancel_btn, &QPushButton::clicked, this, &ApplyFriend::SlotApplyCancel);
    connect(ui->confirm_btn, &QPushButton::clicked, this, &ApplyFriend::SlotApplySure);
}

ApplyFriend::~ApplyFriend() {
    delete ui;
}

void ApplyFriend::InitTipLbs()
{
    int lines = 1;
    for(int i = 0; i < _tip_data.size(); i++){

        auto* lb = new ClickedLabel(ui->label_list);
        lb->SetState("normal", "hover", "pressed", "selected_normal",
            "selected_hover", "selected_pressed");
        lb->setObjectName("tipslb");
        lb->setText(_tip_data[i]);
        connect(lb, &ClickedLabel::clicked, this, &ApplyFriend::SlotChangeFriendLabelByTip);

        QFontMetrics fontMetrics(lb->font()); // 鑾峰彇QLabel鎺т欢鐨勫瓧浣撲俊鎭?
        int textWidth = fontMetrics.horizontalAdvance(lb->text()); // 鑾峰彇鏂囨湰鐨勫搴?
        int textHeight = fontMetrics.height(); // 鑾峰彇鏂囨湰鐨勯珮搴?

        if (_tip_cur_point.x() + textWidth + tip_offset > ui->label_list->width()) {
            lines++;
            if (lines > 2) {
                delete lb;
                return;
            }

            _tip_cur_point.setX(tip_offset);
            _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 15);

        }

        auto next_point = _tip_cur_point;

        AddTipLbs(lb, _tip_cur_point,next_point, textWidth, textHeight);

        _tip_cur_point = next_point;
    }

}

void ApplyFriend::AddTipLbs(ClickedLabel* lb, QPoint cur_point, QPoint& next_point, int text_width, int text_height)
{
    lb->move(cur_point);
    lb->show();
    _add_labels.insert(lb->text(), lb);
    _add_label_keys.push_back(lb->text());
    next_point.setX(lb->pos().x() + text_width + 15);
    next_point.setY(lb->pos().y());
}

bool ApplyFriend::eventFilter(QObject *obj, QEvent *event)
{
    return QDialog::eventFilter(obj, event);
}

void ApplyFriend::SetSearchInfo(std::shared_ptr<SearchInfo> si)
{
    _si = si;
    auto applyname = UserMgr::instance()->GetName();
    auto bakname = si->_name;
    ui->name_edit->setText(applyname);
    ui->back_edit->setText(bakname);
}

void ApplyFriend::ShowMoreLabel()
{
    qDebug()<< "receive more label clicked";
    ui->more_label_wid->hide();

    ui->label_list->setFixedWidth(325);
    _tip_cur_point = QPoint(5, 5);
    auto next_point = _tip_cur_point;
    int textWidth;
    int textHeight;
    //閲嶆媿鐜版湁鐨刲abel
    for(auto & added_key : _add_label_keys){
        auto added_lb = _add_labels[added_key];

        QFontMetrics fontMetrics(added_lb->font()); // 鑾峰彇QLabel鎺т欢鐨勫瓧浣撲俊鎭?
        textWidth = fontMetrics.horizontalAdvance(added_lb->text()); // 鑾峰彇鏂囨湰鐨勫搴?
        textHeight = fontMetrics.height(); // 鑾峰彇鏂囨湰鐨勯珮搴?

        if(_tip_cur_point.x() +textWidth + tip_offset > ui->label_list->width()){
            _tip_cur_point.setX(tip_offset);
            _tip_cur_point.setY(_tip_cur_point.y()+textHeight+15);
        }
        added_lb->move(_tip_cur_point);

        next_point.setX(added_lb->pos().x() + textWidth + 15);
        next_point.setY(_tip_cur_point.y());

        _tip_cur_point = next_point;

    }

    //娣诲姞鏈坊鍔犵殑
    for(int i = 0; i < _tip_data.size(); i++){
        auto iter = _add_labels.find(_tip_data[i]);
        if(iter != _add_labels.end()){
            continue;
        }

        auto* lb = new ClickedLabel(ui->label_list);
        lb->SetState("normal", "hover", "pressed", "selected_normal",
            "selected_hover", "selected_pressed");
        lb->setObjectName("tipslb");
        lb->setText(_tip_data[i]);
        connect(lb, &ClickedLabel::clicked, this, &ApplyFriend::SlotChangeFriendLabelByTip);

        QFontMetrics fontMetrics(lb->font()); // 鑾峰彇QLabel鎺т欢鐨勫瓧浣撲俊鎭?
        int textWidth = fontMetrics.horizontalAdvance(lb->text()); // 鑾峰彇鏂囨湰鐨勫搴?
        int textHeight = fontMetrics.height(); // 鑾峰彇鏂囨湰鐨勯珮搴?

        if (_tip_cur_point.x() + textWidth + tip_offset > ui->label_list->width()) {

            _tip_cur_point.setX(tip_offset);
            _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 15);

        }

         next_point = _tip_cur_point;

        AddTipLbs(lb, _tip_cur_point, next_point, textWidth, textHeight);

        _tip_cur_point = next_point;

    }

   int diff_height = next_point.y() + textHeight + tip_offset - ui->label_list->height();
   ui->label_list->setFixedHeight(next_point.y() + textHeight + tip_offset);

    //qDebug()<<"after resize ui->label_list size is " <<  ui->label_list->size();
    ui->scrollAreaWidgetContents->setFixedHeight(ui->scrollAreaWidgetContents->height()+diff_height);
}

void ApplyFriend::resetLabels()
{
    auto max_width = ui->grid_wid->width();
    auto label_height = 0;
    for(auto iter = _friend_labels.begin(); iter != _friend_labels.end(); iter++){
        //todo... 娣诲姞瀹藉害缁熻
        if( _label_point.x() + iter.value()->width() > max_width) {
            _label_point.setY(_label_point.y()+iter.value()->height()+6);
            _label_point.setX(2);
        }

        iter.value()->move(_label_point);
        iter.value()->show();

        _label_point.setX(_label_point.x()+iter.value()->width()+2);
        _label_point.setY(_label_point.y());
        label_height = iter.value()->height();
    }

    if(_friend_labels.isEmpty()){
        ui->label_edit->move(_label_point);
        ui->label_edit->setFixedWidth(ui->grid_wid->width() - 4);
        ui->grid_wid->setFixedHeight(qMax(30, ui->label_edit->height() + 6));
        return;
    }

    if(_label_point.x() + MIN_APPLY_LABEL_ED_LEN > ui->grid_wid->width()){
        ui->label_edit->move(2,_label_point.y()+label_height+6);
        ui->label_edit->setFixedWidth(ui->grid_wid->width() - 4);
    }else{
        ui->label_edit->move(_label_point);
        ui->label_edit->setFixedWidth(qMax(130, ui->grid_wid->width() - _label_point.x() - 2));
    }
    int needed = qMax(_label_point.y(), ui->label_edit->pos().y()) + qMax(label_height, ui->label_edit->height()) + 6;
    if (ui->grid_wid->height() < needed || _friend_labels.size() < 2) {
        ui->grid_wid->setFixedHeight(qMax(30, needed));
    }
}

void ApplyFriend::addLabel(QString name)
{
    if (_friend_labels.find(name) != _friend_labels.end()) {
        return;
    }

    auto tmplabel = new FriendLabel(ui->grid_wid);
    tmplabel->show();
    tmplabel->SetText(name);
    tmplabel->setObjectName("FriendLabel");

    auto max_width = ui->grid_wid->width();
    //todo... 娣诲姞瀹藉害缁熻
    if (_label_point.x() + tmplabel->width() > max_width) {
        _label_point.setY(_label_point.y() + tmplabel->height() + 6);
        _label_point.setX(2);
    }
    else {

    }


    tmplabel->move(_label_point);
    tmplabel->show();
    _friend_labels[tmplabel->Text()] = tmplabel;
    _friend_label_keys.push_back(tmplabel->Text());

    connect(tmplabel, &FriendLabel::sig_close, this, &ApplyFriend::SlotRemoveFriendLabel);

    _label_point.setX(_label_point.x() + tmplabel->width() + 2);

    if (_label_point.x() + MIN_APPLY_LABEL_ED_LEN > ui->grid_wid->width()) {
        ui->label_edit->move(2, _label_point.y() + tmplabel->height() + 2);
        ui->label_edit->setFixedWidth(ui->grid_wid->width() - 4);
    }
    else {
        ui->label_edit->move(_label_point);
        ui->label_edit->setFixedWidth(qMax(130, ui->grid_wid->width() - _label_point.x() - 2));
    }

    ui->label_edit->clear();

    int needed = qMax(_label_point.y(), ui->label_edit->pos().y()) + qMax(tmplabel->height(), ui->label_edit->height()) + 6;
    if (ui->grid_wid->height() < needed) {
        ui->grid_wid->setFixedHeight(needed);
    }
}

void ApplyFriend::SlotLabelEnter()
{
    if(ui->label_edit->text().isEmpty()){
        return;
    }

    auto text = ui->label_edit->text();
    addLabel(ui->label_edit->text());

    ui->input_tip_wid->hide();
    auto find_it = std::find(_tip_data.begin(), _tip_data.end(), text);
    //鎵惧埌浜嗗氨鍙渶璁剧疆鐘舵€佷负閫変腑鍗冲彲
    if (find_it == _tip_data.end()) {
        _tip_data.push_back(text);
    }

    //鍒ゆ柇鏍囩灞曠ず鏍忔槸鍚︽湁璇ユ爣绛?
    auto find_add = _add_labels.find(text);
    if (find_add != _add_labels.end()) {
        find_add.value()->SetCurState(ClickLbState::Selected);
        return;
    }

    //鏍囩灞曠ず鏍忎篃澧炲姞涓€涓爣绛? 骞惰缃豢鑹查€変腑
    auto* lb = new ClickedLabel(ui->label_list);
    lb->SetState("normal", "hover", "pressed", "selected_normal",
        "selected_hover", "selected_pressed");
    lb->setObjectName("tipslb");
    lb->setText(text);
    connect(lb, &ClickedLabel::clicked, this, &ApplyFriend::SlotChangeFriendLabelByTip);
    qDebug() << "ui->label_list->width() is " << ui->label_list->width();
    qDebug() << "_tip_cur_point.x() is " << _tip_cur_point.x();

    QFontMetrics fontMetrics(lb->font()); // 鑾峰彇QLabel鎺т欢鐨勫瓧浣撲俊鎭?
    int textWidth = fontMetrics.horizontalAdvance(lb->text()); // 鑾峰彇鏂囨湰鐨勫搴?
    int textHeight = fontMetrics.height(); // 鑾峰彇鏂囨湰鐨勯珮搴?
    qDebug() << "textWidth is " << textWidth;

    if (_tip_cur_point.x() + textWidth + tip_offset + 3 > ui->label_list->width()) {

        _tip_cur_point.setX(5);
        _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 15);

    }

    auto next_point = _tip_cur_point;

    AddTipLbs(lb, _tip_cur_point, next_point, textWidth, textHeight);
    _tip_cur_point = next_point;

    int diff_height = next_point.y() + textHeight + tip_offset - ui->label_list->height();
    ui->label_list->setFixedHeight(next_point.y() + textHeight + tip_offset);

    lb->SetCurState(ClickLbState::Selected);

    ui->scrollAreaWidgetContents->setFixedHeight(ui->scrollAreaWidgetContents->height() + diff_height);
}

void ApplyFriend::SlotRemoveFriendLabel(QString name)
{
    qDebug() << "receive close signal";

    _label_point.setX(2);
    _label_point.setY(6);

    auto find_iter = _friend_labels.find(name);

    if(find_iter == _friend_labels.end()){
        return;
    }

    auto find_key = _friend_label_keys.end();
    for(auto iter = _friend_label_keys.begin(); iter != _friend_label_keys.end();
        iter++){
        if(*iter == name){
            find_key = iter;
            break;
        }
        }

    if(find_key != _friend_label_keys.end()){
        _friend_label_keys.erase(find_key);
    }


    find_iter.value()->deleteLater();

    _friend_labels.erase(find_iter);

    resetLabels();

    auto find_add = _add_labels.find(name);
    if(find_add == _add_labels.end()){
        return;
    }

    find_add.value()->ResetNormalState();
}

//鐐瑰嚮鏍囧凡鏈夌娣诲姞鎴栧垹闄ゆ柊鑱旂郴浜虹殑鏍囩
void ApplyFriend::SlotChangeFriendLabelByTip(QString lbtext, ClickLbState state)
{
    auto find_iter = _add_labels.find(lbtext);
    if(find_iter == _add_labels.end()){
        return;
    }

    if(state == ClickLbState::Selected){
        //缂栧啓娣诲姞閫昏緫
        addLabel(lbtext);
        return;
    }

    if(state == ClickLbState::Normal){
        //缂栧啓鍒犻櫎閫昏緫
        SlotRemoveFriendLabel(lbtext);
        return;
    }

}

void ApplyFriend::SlotLabelTextChange(const QString& text)
{
    if (text.isEmpty()) {
        ui->tip_label->setText("");
        ui->input_tip_wid->hide();
        return;
    }

    auto iter = std::find(_tip_data.begin(), _tip_data.end(), text);
    if (iter == _tip_data.end()) {
        auto new_text = add_prefix + text;
        ui->tip_label->setText(new_text);
        ui->input_tip_wid->show();
        return;
    }
    ui->tip_label->setText(text);
    ui->input_tip_wid->show();
}

void ApplyFriend::SlotLabelEditFinished()
{
    ui->input_tip_wid->hide();
}

void ApplyFriend::SlotAddFirendLabelByClickTip(QString text)
{
    int index = text.indexOf(add_prefix);
    if (index != -1) {
        text = text.mid(index + add_prefix.length());
    }
    addLabel(text);

    auto find_it = std::find(_tip_data.begin(), _tip_data.end(), text);
    //鎵惧埌浜嗗氨鍙渶璁剧疆鐘舵€佷负閫変腑鍗冲彲
    if (find_it == _tip_data.end()) {
        _tip_data.push_back(text);
    }

    //鍒ゆ柇鏍囩灞曠ず鏍忔槸鍚︽湁璇ユ爣绛?
    auto find_add = _add_labels.find(text);
    if (find_add != _add_labels.end()) {
        find_add.value()->SetCurState(ClickLbState::Selected);
        return;
    }

    //鏍囩灞曠ず鏍忎篃澧炲姞涓€涓爣绛? 骞惰缃豢鑹查€変腑
    auto* lb = new ClickedLabel(ui->label_list);
    lb->SetState("normal", "hover", "pressed", "selected_normal",
        "selected_hover", "selected_pressed");
    lb->setObjectName("tipslb");
    lb->setText(text);
    connect(lb, &ClickedLabel::clicked, this, &ApplyFriend::SlotChangeFriendLabelByTip);
    qDebug() << "ui->label_list->width() is " << ui->label_list->width();
    qDebug() << "_tip_cur_point.x() is " << _tip_cur_point.x();

    QFontMetrics fontMetrics(lb->font()); // 鑾峰彇QLabel鎺т欢鐨勫瓧浣撲俊鎭?
    int textWidth = fontMetrics.horizontalAdvance(lb->text()); // 鑾峰彇鏂囨湰鐨勫搴?
    int textHeight = fontMetrics.height(); // 鑾峰彇鏂囨湰鐨勯珮搴?
    qDebug() << "textWidth is " << textWidth;

    if (_tip_cur_point.x() + textWidth+ tip_offset+3 > ui->label_list->width()) {

        _tip_cur_point.setX(5);
        _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 15);

    }

    auto next_point = _tip_cur_point;

    AddTipLbs(lb, _tip_cur_point, next_point, textWidth,textHeight);
    _tip_cur_point = next_point;

    int diff_height = next_point.y() + textHeight + tip_offset - ui->label_list->height();
    ui->label_list->setFixedHeight(next_point.y() + textHeight + tip_offset);

    lb->SetCurState(ClickLbState::Selected);

    ui->scrollAreaWidgetContents->setFixedHeight(ui->scrollAreaWidgetContents->height()+ diff_height );
}

void ApplyFriend::SlotApplyCancel()
{
    qDebug() << "Slot Apply Cancel";
    this->hide();
    deleteLater();
}

void ApplyFriend::SlotApplySure()
{
    QJsonObject jsonObj;
    auto uid = UserMgr::instance()->GetUid();
    jsonObj["uid"] = uid;
    auto name = ui->name_edit->text();
    jsonObj["applyname"] = name;
    auto bakname = ui->back_edit->text();
    jsonObj["bakname"] = bakname;
    jsonObj["touid"] = _si->_uid;

    QJsonDocument doc(jsonObj);
    QByteArray jsonData = QJsonDocument(doc).toJson(QJsonDocument::Compact);
    emit TCPMgr::instance()->sig_send_data(ReqId::ID_ADD_FRIEND_REQ, jsonData);

    this->hide();
    deleteLater();
}