// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_xcmd.h -- extended command widget

#ifndef QT4XCMD_H
#define QT4XCMD_H

namespace nethack_qt_ {

enum xcmdSets { all_cmds = 0, normal_cmds = 1, wizard_cmds = 2 };
enum xcmdMisc { xcmdNone = -10, xcmdNoMatch = 9999 };

class NetHackQtExtCmdRequestor : public QDialog {
    Q_OBJECT

protected:
    virtual void keyPressEvent(QKeyEvent *event);

public:
    NetHackQtExtCmdRequestor(QWidget *parent);
    int get();

private:
    QLabel *prompt;
    QPushButton *cancel_btn;
    QVector<QPushButton *> buttons;
    bool byRow;       // local copy of qt_settings->xcmd_by_row;
    int set;          // local copy of qt_settings->xcmd_set;
    int butoffset;    // number of control buttons (cancel, filter, &c)
    unsigned exactmatchindx;

    void enableButtons();
    void Cancel();    // not selecting a command after all
    void Filter();    // choose command set (all, normal mode, wizard mode)
    void Layout();    // by-column vs by-row for button grid
    void Reset();     // go back to default filter and layout

    void Retry();     // returns to caller in order to be called back...
                      // ...and restart with revised settings

    inline void DefaultActionIsCancel(bool make_it_so,
                                      unsigned matchindx = xcmdNoMatch)
    {
        if (!make_it_so ^ !cancel_btn->isDefault()) {
            cancel_btn->setDefault(make_it_so);
            exactmatchindx = matchindx;
        }
    }

private slots:
    int Button(int);  // click handler
};

} // namespace nethack_qt_

#endif
