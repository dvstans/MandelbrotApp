#ifndef PALETTEEDITDIALOG_H
#define PALETTEEDITDIALOG_H

#include <QDialog>
#include <QCloseEvent>

namespace Ui {
class PaletteEditDialog;
}

class PaletteEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PaletteEditDialog(QWidget *parent = nullptr);
    ~PaletteEditDialog();

public slots:
    void showWithPos();
    void hideWithPos();

private:
    void    closeEvent( QCloseEvent *event );
    void    insertColorControls( int pos );

    Ui::PaletteEditDialog*  ui;
    QRect                   m_geometry;
    bool                    m_geometry_set;
};

#endif // PALETTEEDITDIALOG_H
