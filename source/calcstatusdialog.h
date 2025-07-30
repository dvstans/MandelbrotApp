#ifndef CALCSTATUSDIALOG_H
#define CALCSTATUSDIALOG_H

#include <QDialog>
#include "mandelbrotcalc.h"

namespace Ui {
class CalcStatusDialog;
}

class CalcStatusDialog : public QDialog, public MandelbrotCalc::IObserver
{
    Q_OBJECT

public:
    explicit CalcStatusDialog( QWidget *parent, MandelbrotCalc::IObserver &a_observer );
    ~CalcStatusDialog();

    void    reset();

private:
    void    calcProgress( int a_progress );

    // MandelbrotCalc::IObserver methods
    void    cbCalcProgress( int a_progress );
    void    cbCalcCompleted( MandelbrotCalc::Result a_result );
    void    cbCalcCancelled();

    Ui::CalcStatusDialog *      ui;
    MandelbrotCalc::IObserver & m_observer;
};

#endif // CALCSTATUSDIALOG_H
