#include <XdgDirs>
#include <QDebug>

int main(int argc, char **argv)
{
    qDebug() << XdgDirs::dataDirs();
    return 0;
}
