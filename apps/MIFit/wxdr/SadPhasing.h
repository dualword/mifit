#ifndef SADPHASING_H
#define SADPHASING_H

#include <string>

#include "corelib.h"

#include "ui_SadPhasing.h"
#include "MIQDialog.h"

class QAbstractButton;

class SadPhasing : public MIQDialog, public Ui::SadPhasing
{
    Q_OBJECT

public:
    SadPhasing(QWidget *parent = 0);
    static void GetInitialData(MIData &dat);

    void InitializeFromData(const MIData &dat);
    bool GetData(MIData &data);

  private Q_SLOTS:
    void validateTimeout();

  private:
    QAbstractButton *_okButton;
};

#endif
