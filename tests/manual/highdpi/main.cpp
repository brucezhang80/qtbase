/****************************************************************************
 **
 ** Copyright (C) 2015 The Qt Company Ltd.
 ** Contact: http://www.qt.io/licensing/
 **
 ** This file is part of the test suite of the Qt Toolkit.
 **
 ** $QT_BEGIN_LICENSE:LGPL21$
 ** Commercial License Usage
 ** Licensees holding valid commercial Qt licenses may use this file in
 ** accordance with the commercial license agreement provided with the
 ** Software or, alternatively, in accordance with the terms contained in
 ** a written agreement between you and The Qt Company. For licensing terms
 ** and conditions see http://www.qt.io/terms-conditions. For further
 ** information use the contact form at http://www.qt.io/contact-us.
 **
 ** GNU Lesser General Public License Usage
 ** Alternatively, this file may be used under the terms of the GNU Lesser
 ** General Public License version 2.1 or version 3 as published by the Free
 ** Software Foundation and appearing in the file LICENSE.LGPLv21 and
 ** LICENSE.LGPLv3 included in the packaging of this file. Please review the
 ** following information to ensure the GNU Lesser General Public License
 ** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
 ** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
 **
 ** As a special exception, The Qt Company gives you certain additional
 ** rights. These rights are described in The Qt Company LGPL Exception
 ** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
 **
 ** $QT_END_LICENSE$
 **
 ****************************************************************************/

#include <QMainWindow>
#include <QLabel>
#include <QHBoxLayout>
#include <QApplication>
#include <QAction>
#include <QStyle>
#include <QToolBar>
#include <QPushButton>
#include <QButtonGroup>
#include <QLineEdit>
#include <QScrollBar>
#include <QSlider>
#include <QSpinBox>
#include <QTabBar>
#include <QIcon>
#include <QPainter>
#include <QWindow>
#include <QScreen>
#include <QFile>
#include <QMouseEvent>
#include <QTemporaryDir>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <private/qhighdpiscaling_p.h>

#include "dragwidget.h"

class DemoContainerBase
{
public:
    virtual ~DemoContainerBase() {}
    QString name() { return option().names().first(); }
    virtual QCommandLineOption &option() = 0;
    virtual void makeVisible(bool visible) = 0;
};

typedef QList<DemoContainerBase*> DemoContainerList ;


template <class T>
class DemoContainer : public DemoContainerBase
{
public:
    DemoContainer(const QString &optionName, const QString &description)
        : m_widget(0), m_option(optionName, description)
    {
    }
    ~DemoContainer() { delete m_widget; }

    QCommandLineOption &option() { return m_option; }

    void makeVisible(bool visible) {
        if (visible && !m_widget)
            m_widget = new T;
        if (m_widget)
            m_widget->setVisible(visible);
    }
private:
    QWidget *m_widget;
    QCommandLineOption m_option;
};

class DemoController : public QWidget
{
Q_OBJECT
public:
    DemoController(DemoContainerList *demos, QCommandLineParser *parser);
    ~DemoController();
private slots:
    void handleButton(int id, bool toggled);
private:
    DemoContainerList *m_demos;
    QButtonGroup *m_group;
};

DemoController::DemoController(DemoContainerList *demos, QCommandLineParser *parser)
    : m_demos(demos)
{
    setWindowTitle("screen scale factors");
    setObjectName("controller"); // make WindowScaleFactorSetter skip this window

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    // set up one scale control line per screen
    QList<QScreen *> screens = QGuiApplication::screens();
    foreach (QScreen *screen, screens) {
        // create scale control line
        QHBoxLayout *row = new QHBoxLayout;
        QSize screenSize = screen->geometry().size();
        QString screenId = screen->name() + " " + QString::number(screenSize.width())
                                          + " " + QString::number(screenSize.height());
        QLabel *label = new QLabel(screenId);
        QSlider *slider = new QSlider();
        slider->setOrientation(Qt::Horizontal);
        slider->setMinimum(1);
        slider->setMaximum(40);
        slider->setValue(10);
        slider->setTracking(false);
        slider->setTickInterval(5);
        slider->setTickPosition(QSlider::TicksBelow);
        QLabel *scaleFactorLabel = new QLabel("1.0");

        // set up layouts
        row->addWidget(label);
        row->addWidget(slider);
        row->addWidget(scaleFactorLabel);
        layout->addLayout(row);

        // handle slider position change
        connect(slider, &QSlider::sliderMoved, [scaleFactorLabel, screen](int scaleFactor){
            // slider value is scale factor times ten;
            qreal scalefactorF = qreal(scaleFactor) / 10.0;

            // update label, add ".0" if needed.
            QString number = QString::number(scalefactorF);
            if (!number.contains("."))
                number.append(".0");
            scaleFactorLabel->setText(number);
            });
        // handle slider value change
        connect(slider, &QSlider::valueChanged, [scaleFactorLabel, screen](int scaleFactor){
            // slider value is scale factor times ten;
            qreal scalefactorF = qreal(scaleFactor) / 10.0;

            // set scale factor for screen
            qreal oldFactor = QHighDpiScaling::factor(screen);
            QHighDpiScaling::setScreenFactor(screen, scalefactorF);
            qreal newFactor = QHighDpiScaling::factor(screen);

            qDebug() << "factor was / is" << oldFactor << newFactor;
        });
    }

    m_group = new QButtonGroup(this);
    m_group->setExclusive(false);

    for (int i = 0; i < m_demos->size(); ++i) {
        DemoContainerBase *demo = m_demos->at(i);
        QPushButton *button = new QPushButton(demo->name());
        button->setToolTip(demo->option().description());
        button->setCheckable(true);
        layout->addWidget(button);
        m_group->addButton(button, i);

        if (parser->isSet(demo->option())) {
            demo->makeVisible(true);
            button->setChecked(true);
        }
    }
    connect(m_group, SIGNAL(buttonToggled(int, bool)), this, SLOT(handleButton(int, bool)));
}

DemoController::~DemoController()
{
    qDeleteAll(*m_demos);
}

void DemoController::handleButton(int id, bool toggled)
{
    m_demos->at(id)->makeVisible(toggled);
}

class PixmapPainter : public QWidget
{
public:
    PixmapPainter();
    void paintEvent(QPaintEvent *event);

    QPixmap pixmap1X;
    QPixmap pixmap2X;
    QPixmap pixmapLarge;
    QImage image1X;
    QImage image2X;
    QImage imageLarge;
    QIcon qtIcon;
};

PixmapPainter::PixmapPainter()
{
    pixmap1X = QPixmap(":/qticon32.png");
    pixmap2X = QPixmap(":/qticon32@2x.png");
    pixmapLarge = QPixmap(":/qticon64.png");

    image1X = QImage(":/qticon32.png");
    image2X = QImage(":/qticon32@2x.png");
    imageLarge = QImage(":/qticon64.png");

    qtIcon.addFile(":/qticon32.png");
    qtIcon.addFile(":/qticon32@2x.png");
}

void PixmapPainter::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(QRect(QPoint(0, 0), size()), QBrush(Qt::gray));

    int pixmapPointSize = 32;
    int y = 30;
    int dy = 90;

    int x = 10;
    int dx = 40;
    // draw at point
//          qDebug() << "paint pixmap" << pixmap1X.devicePixelRatio();
          p.drawPixmap(x, y, pixmap1X);
    x+=dx;p.drawPixmap(x, y, pixmap2X);
    x+=dx;p.drawPixmap(x, y, pixmapLarge);
  x+=dx*2;p.drawPixmap(x, y, qtIcon.pixmap(QSize(pixmapPointSize, pixmapPointSize)));
    x+=dx;p.drawImage(x, y, image1X);
    x+=dx;p.drawImage(x, y, image2X);
    x+=dx;p.drawImage(x, y, imageLarge);

    // draw at 32x32 rect
    y+=dy;
    x = 10;
          p.drawPixmap(QRect(x, y, pixmapPointSize, pixmapPointSize), pixmap1X);
    x+=dx;p.drawPixmap(QRect(x, y, pixmapPointSize, pixmapPointSize), pixmap2X);
    x+=dx;p.drawPixmap(QRect(x, y, pixmapPointSize, pixmapPointSize), pixmapLarge);
    x+=dx;p.drawPixmap(QRect(x, y, pixmapPointSize, pixmapPointSize), qtIcon.pixmap(QSize(pixmapPointSize, pixmapPointSize)));
    x+=dx;p.drawImage(QRect(x, y, pixmapPointSize, pixmapPointSize), image1X);
    x+=dx;p.drawImage(QRect(x, y, pixmapPointSize, pixmapPointSize), image2X);
    x+=dx;p.drawImage(QRect(x, y, pixmapPointSize, pixmapPointSize), imageLarge);


    // draw at 64x64 rect
    y+=dy - 50;
    x = 10;
               p.drawPixmap(QRect(x, y, pixmapPointSize * 2, pixmapPointSize * 2), pixmap1X);
    x+=dx * 2; p.drawPixmap(QRect(x, y, pixmapPointSize * 2, pixmapPointSize * 2), pixmap2X);
    x+=dx * 2; p.drawPixmap(QRect(x, y, pixmapPointSize * 2, pixmapPointSize * 2), pixmapLarge);
    x+=dx * 2; p.drawPixmap(QRect(x, y, pixmapPointSize * 2, pixmapPointSize * 2), qtIcon.pixmap(QSize(pixmapPointSize, pixmapPointSize)));
    x+=dx * 2; p.drawImage(QRect(x, y, pixmapPointSize * 2, pixmapPointSize * 2), image1X);
    x+=dx * 2; p.drawImage(QRect(x, y, pixmapPointSize * 2, pixmapPointSize * 2), image2X);
    x+=dx * 2; p.drawImage(QRect(x, y, pixmapPointSize * 2, pixmapPointSize * 2), imageLarge);
 }

class Labels : public QWidget
{
public:
    Labels();

    QPixmap pixmap1X;
    QPixmap pixmap2X;
    QPixmap pixmapLarge;
    QIcon qtIcon;
};

Labels::Labels()
{
    pixmap1X = QPixmap(":/qticon32.png");
    pixmap2X = QPixmap(":/qticon32@2x.png");
    pixmapLarge = QPixmap(":/qticon64.png");

    qtIcon.addFile(":/qticon32.png");
    qtIcon.addFile(":/qticon32@2x.png");
    setWindowIcon(qtIcon);
    setWindowTitle("Labels");

    QLabel *label1x = new QLabel();
    label1x->setPixmap(pixmap1X);
    QLabel *label2x = new QLabel();
    label2x->setPixmap(pixmap2X);
    QLabel *labelIcon = new QLabel();
    labelIcon->setPixmap(qtIcon.pixmap(QSize(32,32)));
    QLabel *labelLarge = new QLabel();
    labelLarge->setPixmap(pixmapLarge);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(label1x);    //expected low-res on high-dpi displays
    layout->addWidget(label2x);    //expected high-res on high-dpi displays
    layout->addWidget(labelIcon);  //expected high-res on high-dpi displays
    layout->addWidget(labelLarge); // expected large size and low-res
    setLayout(layout);
}

class MainWindow : public QMainWindow
{
public:
    MainWindow();

    QIcon qtIcon;
    QIcon qtIcon1x;
    QIcon qtIcon2x;

    QToolBar *fileToolBar;
};

MainWindow::MainWindow()
{
    // beware that QIcon auto-loads the @2x versions.
    qtIcon1x.addFile(":/qticon16.png");
    qtIcon2x.addFile(":/qticon32.png");
    setWindowIcon(qtIcon);
    setWindowTitle("MainWindow");

    fileToolBar = addToolBar(tr("File"));
//    fileToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    fileToolBar->addAction(new QAction(qtIcon1x, QString("1x"), this));
    fileToolBar->addAction(new QAction(qtIcon2x, QString("2x"), this));
}

class StandardIcons : public QWidget
{
public:
    void paintEvent(QPaintEvent *)
    {
        int x = 10;
        int y = 10;
        int dx = 50;
        int dy = 50;
        int maxX = 500;

        for (uint iconIndex = QStyle::SP_TitleBarMenuButton; iconIndex < QStyle::SP_MediaVolumeMuted; ++iconIndex) {
            QIcon icon = qApp->style()->standardIcon(QStyle::StandardPixmap(iconIndex));
            QPainter p(this);
            p.drawPixmap(x, y, icon.pixmap(dx - 5, dy - 5));
            if (x + dx > maxX)
                y+=dy;
            x = ((x + dx) % maxX);
        }
    }
};

class Caching : public QWidget
{
public:
    void paintEvent(QPaintEvent *)
    {
        QSize layoutSize(75, 75);

        QPainter widgetPainter(this);
        widgetPainter.fillRect(QRect(QPoint(0, 0), this->size()), Qt::gray);

        {
            const qreal devicePixelRatio = this->windowHandle()->devicePixelRatio();
            QPixmap cache(layoutSize * devicePixelRatio);
            cache.setDevicePixelRatio(devicePixelRatio);

            QPainter cachedPainter(&cache);
            cachedPainter.fillRect(QRect(0,0, 75, 75), Qt::blue);
            cachedPainter.fillRect(QRect(10,10, 55, 55), Qt::red);
            cachedPainter.drawEllipse(QRect(10,10, 55, 55));

            QPainter widgetPainter(this);
            widgetPainter.drawPixmap(QPoint(10, 10), cache);
        }

        {
            const qreal devicePixelRatio = this->windowHandle()->devicePixelRatio();
            QImage cache = QImage(layoutSize * devicePixelRatio, QImage::Format_ARGB32_Premultiplied);
            cache.setDevicePixelRatio(devicePixelRatio);

            QPainter cachedPainter(&cache);
            cachedPainter.fillRect(QRect(0,0, 75, 75), Qt::blue);
            cachedPainter.fillRect(QRect(10,10, 55, 55), Qt::red);
            cachedPainter.drawEllipse(QRect(10,10, 55, 55));

            QPainter widgetPainter(this);
            widgetPainter.drawImage(QPoint(95, 10), cache);
        }

    }
};

class Style : public QWidget {
public:
    QPushButton *button;
    QLineEdit *lineEdit;
    QSlider *slider;
    QHBoxLayout *row1;

    Style() {
        row1 = new QHBoxLayout();
        setLayout(row1);

        button = new QPushButton();
        button->setText("Test Button");
        row1->addWidget(button);

        lineEdit = new QLineEdit();
        lineEdit->setText("Test Lineedit");
        row1->addWidget(lineEdit);

        slider = new QSlider();
        row1->addWidget(slider);

        row1->addWidget(new QSpinBox);
        row1->addWidget(new QScrollBar);

        QTabBar *tab  = new QTabBar();
        tab->addTab("Foo");
        tab->addTab("Bar");
        row1->addWidget(tab);
    }
};

class Fonts : public QWidget
{
public:
    void paintEvent(QPaintEvent *)
    {
        QPainter painter(this);

        // Points
        int y = 10;
        for (int fontSize = 6; fontSize < 18; fontSize += 2) {
            QFont font;
            font.setPointSize(fontSize);
            QString string = QString(QStringLiteral("%1 The quick brown fox jumped over the lazy Doug.")).arg(fontSize);
            painter.setFont(font);
            painter.drawText(10, y, string);
            y += (fontSize  * 2.5);
        }

        // Pixels
        y = 160;
        for (int fontSize = 6; fontSize < 18; fontSize += 2) {
            QFont font;
            font.setPixelSize(fontSize);
            QString string = QString(QStringLiteral("%1 The quick brown fox jumped over the lazy Doug.")).arg(fontSize);
            painter.setFont(font);
            painter.drawText(10, y, string);
            y += (fontSize  * 2.5);
        }
    }
};


template <typename T>
void apiTestdevicePixelRatioGetter()
{
    if (0) {
        T *t = 0;
        t->devicePixelRatio();
    }
}

template <typename T>
void apiTestdevicePixelRatioSetter()
{
    if (0) {
        T *t = 0;
        t->setDevicePixelRatio(2.0);
    }
}

void apiTest()
{
    // compile call to devicePixelRatio getter and setter (verify spelling)
    apiTestdevicePixelRatioGetter<QWindow>();
    apiTestdevicePixelRatioGetter<QScreen>();
    apiTestdevicePixelRatioGetter<QGuiApplication>();

    apiTestdevicePixelRatioGetter<QImage>();
    apiTestdevicePixelRatioSetter<QImage>();
    apiTestdevicePixelRatioGetter<QPixmap>();
    apiTestdevicePixelRatioSetter<QPixmap>();
}

// Request and draw an icon at different sizes
class IconDrawing : public QWidget
{
public:
    IconDrawing()
    {
        const QString tempPath = m_temporaryDir.path();
        const QString path32 = tempPath + "/qticon32.png";
        const QString path32_2 = tempPath + "/qticon32-2.png";
        const QString path32_2x = tempPath + "/qticon32@2x.png";

        QFile::copy(":/qticon32.png", path32_2);
        QFile::copy(":/qticon32.png", path32);
        QFile::copy(":/qticon32@2x.png", path32_2x);

        iconHighDPI.reset(new QIcon(path32)); // will auto-load @2x version.
        iconNormalDpi.reset(new QIcon(path32_2)); // does not have a 2x version.
    }

    void paintEvent(QPaintEvent *)
    {
        int x = 10;
        int y = 10;
        int dx = 50;
        int dy = 50;
        int maxX = 600;
        int minSize = 5;
        int maxSize = 64;
        int sizeIncrement = 5;

        // Disable high-dpi icons
        QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, false);

        // normal icon
        for (int size = minSize; size < maxSize; size += sizeIncrement) {
            QPainter p(this);
            p.drawPixmap(x, y, iconNormalDpi->pixmap(size, size));
            if (x + dx > maxX)
                y+=dy;
            x = ((x + dx) % maxX);
        }
        x = 10;
        y+=dy;

        // high-dpi icon
        for (int size = minSize; size < maxSize; size += sizeIncrement) {
            QPainter p(this);
            p.drawPixmap(x, y, iconHighDPI->pixmap(size, size));
            if (x + dx > maxX)
                y+=dy;
            x = ((x + dx) % maxX);
        }

        x = 10;
        y+=dy;

        // Enable high-dpi icons
        QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);

        // normal icon
        for (int size = minSize; size < maxSize; size += sizeIncrement) {
            QPainter p(this);
            p.drawPixmap(x, y, iconNormalDpi->pixmap(size, size));
            if (x + dx > maxX)
                y+=dy;
            x = ((x + dx) % maxX);
        }
        x = 10;
        y+=dy;

        // high-dpi icon (draw point)
        for (int size = minSize; size < maxSize; size += sizeIncrement) {
            QPainter p(this);
            p.drawPixmap(x, y, iconHighDPI->pixmap(size, size));
            if (x + dx > maxX)
                y+=dy;
            x = ((x + dx) % maxX);
        }

        x = 10;
        y+=dy;

    }

private:
    QTemporaryDir m_temporaryDir;
    QScopedPointer<QIcon> iconHighDPI;
    QScopedPointer<QIcon> iconNormalDpi;
};

// Icons on buttons
class Buttons : public QWidget
{
public:
    Buttons()
    {
        QIcon icon;
        icon.addFile(":/qticon16@2x.png");

        QPushButton *button =  new QPushButton(this);
        button->setIcon(icon);
        button->setText("16@2x");

        QTabBar *tab = new QTabBar(this);
        tab->addTab(QIcon(":/qticon16.png"), "16@1x");
        tab->addTab(QIcon(":/qticon16@2x.png"), "16@2x");
        tab->addTab(QIcon(":/qticon16.png"), "");
        tab->addTab(QIcon(":/qticon16@2x.png"), "");
        tab->move(10, 100);
        tab->show();

        QToolBar *toolBar = new QToolBar(this);
        toolBar->addAction(QIcon(":/qticon16.png"), "16");
        toolBar->addAction(QIcon(":/qticon16@2x.png"), "16@2x");
        toolBar->addAction(QIcon(":/qticon32.png"), "32");
        toolBar->addAction(QIcon(":/qticon32@2x.png"), "32@2x");

        toolBar->move(10, 200);
        toolBar->show();
    }
};

class LinePainter : public QWidget
{
public:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

    QPoint lastMousePoint;
    QVector<QPoint> linePoints;
};

void LinePainter::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(QRect(QPoint(0, 0), size()), QBrush(Qt::gray));

    // Default antialiased line
    p.setRenderHint(QPainter::Antialiasing);
    p.drawLines(linePoints);

    // Cosmetic 1 antialiased line
    QPen pen;
    pen.setCosmetic(true);
    pen.setWidth(1);
    p.setPen(pen);
    p.translate(3, 3);
    p.drawLines(linePoints);

    // Aliased cosmetic 1 line
    p.setRenderHint(QPainter::Antialiasing, false);
    p.translate(3, 3);
    p.drawLines(linePoints);
}

void LinePainter::mousePressEvent(QMouseEvent *event)
{
    lastMousePoint = event->pos();
}

void LinePainter::mouseReleaseEvent(QMouseEvent *)
{
    lastMousePoint = QPoint();
}

void LinePainter::mouseMoveEvent(QMouseEvent *event)
{
    if (lastMousePoint.isNull())
        return;

    QPoint newMousePoint = event->pos();
    if (lastMousePoint == newMousePoint)
        return;
    linePoints.append(lastMousePoint);
    linePoints.append(newMousePoint);
    lastMousePoint = newMousePoint;
    update();
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);

    int argumentCount = QCoreApplication::arguments().count();

    QCommandLineParser parser;
    parser.setApplicationDescription("High DPI tester. Pass one or more of the options to\n"
                                     "test various high-dpi aspects. \n"
                                     "--interactive is a special option and opens a configuration"
                                     " window.");
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption controllerOption("interactive", "Show configuration window.");
    parser.addOption(controllerOption);


    DemoContainerList demoList;
    demoList << new DemoContainer<PixmapPainter>("pixmap", "Test pixmap painter");
    demoList << new DemoContainer<Labels>("label", "Test Labels");
    demoList << new DemoContainer<MainWindow>("mainwindow", "Test QMainWindow");
    demoList << new DemoContainer<StandardIcons>("standard-icons", "Test standard icons");
    demoList << new DemoContainer<Caching>("caching", "Test caching");
    demoList << new DemoContainer<Style>("styles", "Test style");
    demoList << new DemoContainer<Fonts>("fonts", "Test fonts");
    demoList << new DemoContainer<IconDrawing>("icondrawing", "Test icon drawing");
    demoList << new DemoContainer<Buttons>("buttons", "Test buttons");
    demoList << new DemoContainer<LinePainter>("linepainter", "Test line painting");
    demoList << new DemoContainer<DragWidget>("draganddrop", "Test drag and drop");


    foreach (DemoContainerBase *demo, demoList)
        parser.addOption(demo->option());

    parser.process(app);

    //controller takes ownership of all demos
    DemoController controller(&demoList, &parser);

    if (parser.isSet(controllerOption) || argumentCount <= 1)
        controller.show();

    if (QApplication::topLevelWidgets().isEmpty())
        parser.showHelp(0);

    return app.exec();
}

#include "main.moc"
