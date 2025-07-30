#include <iostream>
#include "calcstatusdialog.h"
#include "ui_calcstatusdialog.h"

using namespace std;

CalcStatusDialog::CalcStatusDialog( QWidget *a_parent, MandelbrotCalc::IObserver &a_observer ) :
    QDialog( a_parent, Qt::CustomizeWindowHint | Qt::WindowTitleHint ),
    ui(new Ui::CalcStatusDialog),
    m_observer( a_observer )
{
    ui->setupUi(this);
}

CalcStatusDialog::~CalcStatusDialog()
{
    delete ui;
}


void
CalcStatusDialog::reset()
{
    show();
    ui->progressBar->setValue( 0 );
}

void
CalcStatusDialog::calcProgress( int a_progress )
{
    ui->progressBar->setValue( a_progress );
}

// MandelbrotCalc::IObserver methods

void
CalcStatusDialog::cbCalcProgress( int a_progress )
{
    QMetaObject::invokeMethod( this, &CalcStatusDialog::calcProgress, a_progress );
}

void
CalcStatusDialog::cbCalcCompleted( MandelbrotCalc::Result a_result )
{
    m_observer.cbCalcCompleted( a_result );
    hide();
}

void
CalcStatusDialog::cbCalcCancelled()
{
    cout << "calc cancelled" << endl;

    m_observer.cbCalcCancelled();
    hide();
}
