/*
 * Copyright (C) by Roeland Jago Douma <roeland@owncloud.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#include "shareusergroupdialog.h"
#include "ui_shareusergroupdialog.h"
#include "ui_sharedialogshare.h"
#include "account.h"
#include "json.h"
#include "folderman.h"
#include "folder.h"
#include "accountmanager.h"
#include "theme.h"
#include "configfile.h"
#include "capabilities.h"

#include "thumbnailjob.h"
#include "share.h"

#include "QProgressIndicator.h"
#include <QBuffer>
#include <QFileIconProvider>
#include <QClipboard>
#include <QFileInfo>

namespace OCC {

ShareUserGroupDialog::ShareUserGroupDialog(AccountPtr account, const QString &sharePath, const QString &localPath, bool resharingAllowed, QWidget *parent) :
   QDialog(parent),
    _ui(new Ui::ShareUserGroupDialog),
    _account(account),
    _sharePath(sharePath),
    _localPath(localPath),
    _resharingAllowed(resharingAllowed)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName("SharingDialogUG"); // required as group for saveGeometry call

    _ui->setupUi(this);

    //Is this a file or folder?
    _isFile = QFileInfo(localPath).isFile();

    _manager = new ShareManager(_account, this);
    connect(_manager, SIGNAL(sharesFetched(QList<QSharedPointer<Share>>)), SLOT(slotSharesFetched(QList<QSharedPointer<Share>>)));
}

void ShareUserGroupDialog::done( int r ) {
    ConfigFile cfg;
    cfg.saveGeometry(this);
    QDialog::done(r);
}

ShareUserGroupDialog::~ShareUserGroupDialog()
{
    delete _ui;
}

void ShareUserGroupDialog::getShares()
{
    _manager->fetchShares(_sharePath);
}

void ShareUserGroupDialog::slotSharesFetched(const QList<QSharedPointer<Share>> &shares)
{
    const QString versionString = _account->serverVersion();
    qDebug() << Q_FUNC_INFO << versionString << "Fetched" << shares.count() << "shares";

    // TODO clear old shares

    foreach(const auto &share, shares) {

        if (share->getShareType() == Share::TypeLink) {
            continue;
        }

        ShareDialogShare *s = new ShareDialogShare(share, this);
        _ui->sharesLayout->addWidget(s);
    }

    // Add all new shares to share list

}

ShareDialogShare::ShareDialogShare(QSharedPointer<Share> share,
                                   QWidget *parent) :
  QWidget(parent),
  _ui(new Ui::ShareDialogShare),
  _share(share)
{
    _ui->setupUi(this);

    if (share->getPermissions() & Share::PermissionUpdate) {
        _ui->permissionUpdate->setCheckState(Qt::Checked);
    }
    if (share->getPermissions() & Share::PermissionCreate) {
        _ui->permissionCreate->setCheckState(Qt::Checked);
    }
    if (share->getPermissions() & Share::PermissionDelete) {
        _ui->permissionDelete->setCheckState(Qt::Checked);
    }
    if (share->getPermissions() & Share::PermissionShare) {
        _ui->permissionShare->setCheckState(Qt::Checked);
    }

    connect(_ui->permissionUpdate, SIGNAL(clicked(bool)), SLOT(slotPermissionsChanged()));
    connect(_ui->permissionCreate, SIGNAL(clicked(bool)), SLOT(slotPermissionsChanged()));
    connect(_ui->permissionDelete, SIGNAL(clicked(bool)), SLOT(slotPermissionsChanged()));
    connect(_ui->permissionShare,  SIGNAL(clicked(bool)), SLOT(slotPermissionsChanged()));

    connect(share.data(), SIGNAL(permissionsSet()), SLOT(slotPermissionsSet()));
    connect(share.data(), SIGNAL(shareDeleted()), SLOT(slotShareDeleted()));
}

void ShareDialogShare::on_deleteShareButton_clicked()
{
    setEnabled(false);
    _share->deleteShare();
}

ShareDialogShare::~ShareDialogShare()
{
    delete _ui;
}

void ShareDialogShare::slotPermissionsChanged()
{
    setEnabled(false);
    
    Share::Permissions permissions = Share::PermissionRead;

    if (_ui->permissionUpdate->checkState() == Qt::Checked) {
        permissions |= Share::PermissionUpdate;
    }

    if (_ui->permissionCreate->checkState() == Qt::Checked) {
        permissions |= Share::PermissionCreate;
    }

    if (_ui->permissionDelete->checkState() == Qt::Checked) {
        permissions |= Share::PermissionDelete;
    }

    if (_ui->permissionShare->checkState() == Qt::Checked) {
        permissions |= Share::PermissionShare;
    }

    _share->setPermissions(permissions);
}

void ShareDialogShare::slotShareDeleted()
{
    deleteLater();
}

void ShareDialogShare::slotPermissionsSet()
{
    setEnabled(true);
}

}