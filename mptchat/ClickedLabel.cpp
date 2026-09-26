//
// Created by mpt on 2026/7/4.
//

#include "clickedlabel.h"
#include <QMouseEvent>
#include <utility>
ClickedLabel::ClickedLabel(QWidget* parent):QLabel (parent),_curstate(ClickLbState::Normal)
{

}


void ClickedLabel::mousePressEvent(QMouseEvent* event)  {
    if (event->button() == Qt::LeftButton) {
        if(_curstate == ClickLbState::Normal){
            qDebug()<<"clicked , change to selected hover: "<< _selected_hover;
            _curstate = ClickLbState::Selected;
            setProperty("state",_selected);
            repolish(this);
            update();
        }else{
            qDebug()<<"clicked , change to normal hover: "<< _normal_hover;
            _curstate = ClickLbState::Normal;
            setProperty("state",_normal);
            repolish(this);
            update();
        }
        emit clicked(this->text(), _curstate);
    }

    QLabel::mousePressEvent(event);
}

// // 澶勭悊榧犳爣鎮仠杩涘叆浜嬩欢
// void ClickedLabel::enterEvent(QEnterEvent *event) {
//     // 鍦ㄨ繖閲屽鐞嗛紶鏍囨偓鍋滆繘鍏ョ殑閫昏緫
//     if(_curstate == ClickLbState::Normal){
//         //qDebug()<<"enter , change to normal hover: "<< _normal_hover;
//         setProperty("state",_normal_hover);
//         repolish(this);
//         update();
//
//     }else{
//         //qDebug()<<"enter , change to selected hover: "<< _selected_hover;
//         setProperty("state",_selected_hover);
//         repolish(this);
//         update();
//     }
//
//     QLabel::enterEvent(event);
// }
//
// // 澶勭悊榧犳爣鎮仠绂诲紑浜嬩欢
// void ClickedLabel::leaveEvent(QEvent* event){
//     // 鍦ㄨ繖閲屽鐞嗛紶鏍囨偓鍋滅寮€鐨勯€昏緫
//     if(_curstate == ClickLbState::Normal){
//         //qDebug()<<"leave , change to normal : "<< _normal;
//         setProperty("state",_normal);
//         repolish(this);
//         update();
//
//     }else{
//         //qDebug()<<"leave , change to normal hover: "<< _selected;
//         setProperty("state",_selected);
//         repolish(this);
//         update();
//     }
//     QLabel::leaveEvent(event);
// }

void ClickedLabel::SetState(QString normal, QString hover, QString press,
                            QString select, QString select_hover, QString select_press)
{
    _normal = std::move(normal);
    _normal_hover = std::move(hover);
    _normal_press = std::move(press);

    _selected = std::move(select);
    _selected_hover = std::move(select_hover);
    _selected_press = std::move(select_press);

    setProperty("state",_normal);
    repolish(this);
}

ClickLbState ClickedLabel::GetCurState(){
    return _curstate;
}

void ClickedLabel::SetCurState(ClickLbState state) {
    _curstate = state;
    if (state == ClickLbState::Selected) {
        setProperty("state", _selected);
    } else {
        setProperty("state", _normal);
    }
    repolish(this);
    update();
}

void ClickedLabel::ResetNormalState() {
    _curstate = ClickLbState::Normal;
    setProperty("state", _normal);
    repolish(this);
    update();
}
