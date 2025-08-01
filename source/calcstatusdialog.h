#ifndef CALCSTATUSDIALOG_H
#define CALCSTATUSDIALOG_H

#include <QDialog>
//#include "mandelbrotcalc.h"

namespace Ui {
class CalcStatusDialog;
}

class CalcStatusDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CalcStatusDialog( QWidget *parent );
    ~CalcStatusDialog();

    QObject*    getCancelBtn();
    void        setProgress( int a_progress );

private:

    Ui::CalcStatusDialog        *ui;
};

#endif // CALCSTATUSDIALOG_H
