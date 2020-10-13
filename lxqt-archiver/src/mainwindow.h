#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QObject>
#include <memory>
#include <vector>
#include <string>

#include "archiver.h"

namespace Ui {
class MainWindow;
}

class QProgressBar;
class QComboBox;
class QStandardItemModel;
class QStandardItem;
class QItemSelection;
class QLineEdit;
class QMenu;
class ArchiverProxyModel;


class MainWindow : public QMainWindow {
    Q_OBJECT
public:

    enum {
        ArchiverItemRole = Qt::UserRole + 1
    };

    enum class ViewMode {
        DirTree,
        FlatList
    };

    explicit MainWindow(QWidget* parent = nullptr);

    ~MainWindow();

    void loadFile(const Fm::FilePath& file);

    std::shared_ptr<Archiver> archiver() const;

    ViewMode viewMode() const;

    void setViewMode(ViewMode viewMode);

    const std::string& currentDirPath() const;

    void chdir(std::string dirPath);

    void chdir(const ArchiverItem* dir);

protected:
    virtual void dropEvent(QDropEvent* event) override;
    virtual void dragEnterEvent(QDragEnterEvent* event) override;

private Q_SLOTS:
    // action slots
    void on_actionCreateNew_triggered(bool checked);

    void on_actionOpen_triggered(bool checked);

    void on_actionArchiveProperties_triggered(bool checked);

    void on_actionAddFiles_triggered(bool checked);

    void on_actionAddFolder_triggered(bool checked);

    void on_actionDelete_triggered(bool checked);

    void on_actionSelectAll_triggered(bool checked);

    void on_actionExtract_triggered(bool checked);

    void on_actionView_triggered(bool checked);

    void on_actionTest_triggered(bool checked);

    void on_actionPassword_triggered(bool checked);

    void on_actionDirTree_toggled(bool checked);

    void on_actionDirTreeMode_toggled(bool checked);

    void on_actionFlatListMode_toggled(bool checked);

    void on_actionExpand_triggered(bool checked);

    void on_actionCollapse_triggered(bool checked);

    void on_actionReload_triggered(bool checked);

    void on_actionStop_triggered(bool checked);

    void on_actionAbout_triggered(bool checked);

    void on_actionFilter_triggered(bool checked);

    void onDirTreeSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

    void onFileListSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

    void onFileListContextMenu(const QPoint &pos);

    void onFileListDoubleClicked(const QModelIndex &index);

    void onFileListActivated(const QModelIndex &index);

    void onFileListEnterPressed();

    void filter(const QString& text);

private Q_SLOTS:
    // Archiver slots

    void onInvalidateContent();

    void onActionStarted(FrAction action);

    void onActionProgress(double fraction);

    void onActionFinished(FrAction action, ArchiverError err);

    void onMessage(QString message);

    void onStoppableChanged(bool stoppable);

    void onPropertiesFileInfoJobFinished();

    void onDragStarted();

private:
    void setFileName(const QString& fileName);

    QList<QStandardItem*> createFileListRow(const ArchiverItem* file);

    void showFileList(const std::vector<const ArchiverItem *>& files);

    void showFlatFileList();

    void showCurrentDirList();

    void setBusyState(bool busy);

    void updateDirTree();

    void buildDirTree(QStandardItem *parent, const ArchiverItem *root);

    void updateUiStates();

    std::vector<const FileData*> selectedFiles(bool recursive);

    const ArchiverItem* itemFromIndex(const QModelIndex& index);

    QModelIndex indexFromItem(const QModelIndex& parent, const ArchiverItem* item);

    void viewSelectedFiles();

    bool isExtracted(const ArchiverItem* item);

private:
    std::unique_ptr<Ui::MainWindow> ui_;
    std::shared_ptr<Archiver> archiver_;
    QProgressBar* progressBar_;
    QLineEdit* currentPathEdit_;
    QMenu* popupMenu_;
    ArchiverProxyModel* proxyModel_;

    std::string currentDirPath_;
    ViewMode viewMode_;
    const ArchiverItem* currentDirItem_;
    std::string password_;
    bool encryptHeader_;
    bool splitVolumes_;
    unsigned int volumeSize_;

    QString tempDir_;
    QStringList launchPaths_;
    QUrl lasrDir_;

    int splitterPos_;
};

#endif // MAINWINDOW_H
