/*******************************************************************************************************
 GridPlugin.h

 nomacs is a fast and small image viewer with the capability of synchronizing multiple instances

 Copyright (C) 2011-2015 Markus Diem <markus@nomacs.org>

 This file is part of nomacs.

 nomacs is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 nomacs is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 *******************************************************************************************************/

#pragma once

#include <QObject>
#include <QtPlugin>
#include <QImage>
#include <QStringList>
#include <QString>
#include <QMessageBox>
#include <QAction>
#include <QLineEdit>
#include <QGraphicsPathItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsBlurEffect>
#include <QToolBar>
#include <QMainWindow>
#include <QColorDialog>
#include <QSpinBox>
#include <QPushButton>
#include <QMouseEvent>

#include "DkPluginInterface.h"
#include "DkNoMacs.h"
#include "DkSettings.h"
#include "DkUtils.h"
#include "DkBaseViewPort.h"
#include "DkImageStorage.h"

namespace nmc {

class GridPluginViewPort;
class GridPluginToolBar;

enum {
	mode_square,
	mode_rect,
};


class GridPlugin : public QObject, DkViewPortInterface {
	Q_OBJECT
	Q_INTERFACES(nmc::DkViewPortInterface)
	Q_PLUGIN_METADATA(IID "com.nomacs.ImageLounge.GridPlugin/3.2" FILE "GridPlugin.json")

public:

	GridPlugin(QObject* parent = 0);
	~GridPlugin();

	QImage image() const override;
	bool hideHUD() const override;

	// QPainterPath getArrowHead(QPainterPath line, const int thickness);
	// QLineF getShorterLine(QPainterPath line, const int thickness);
	// void getBlur(QPainterPath rect, QPainter *painter, QImage &img, int radius);

	QSharedPointer<nmc::DkImageContainer> runPlugin(const QString &runID = QString(), QSharedPointer<nmc::DkImageContainer> imgC = QSharedPointer<nmc::DkImageContainer>()) const override;
	enum {
		ID_ACTION1,
		// add actions here

		id_end
	};

	nmc::DkPluginViewPort* getViewPort() override;
	bool createViewPort(QWidget* parent) override;
	
	void setVisible(bool visible) override;

	GridPluginViewPort* getGridPluginViewPort();

protected:

	nmc::DkPluginViewPort* viewport;
};


class GridPluginViewPort : public nmc::DkPluginViewPort {
	Q_OBJECT

public:
	GridPluginViewPort(QWidget* parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
	~GridPluginViewPort();
	
	QBrush getBrush() const;
	QPen getPen() const;
	bool isCanceled();
	QImage getPaintedImage();
	void clear();

public slots:
	void setBrush(const QBrush& brush);
	void setPen(const QPen& pen);
	void setPenWidth(int width);
	void setPenColor(QColor color);
	void setPanning(bool checked);
	void applyChangesAndClose();
	void discardChangesAndClose();
	virtual void setVisible(bool visible);
	void undoLastPaint();

signals:
	void editShowSignal(bool show);

protected slots:
	void toggleSquare(bool checked);
	void textChange(const QString &text);
	void textEditFinsh();

protected:
	void mouseMoveEvent(QMouseEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent*event);
	void paintEvent(QPaintEvent *event);
	void addRiceGrid(QPointF begin, QPointF point);
	virtual void init();

	void loadSettings();
	void saveSettings() const;

	QVector<QPainterPath> paths;
	QVector<QPen> pathsPen;
	QVector<int> pathsMode;
	QPointF begin;
	QString sbuffer;

	int selectedMode;
	bool textinputenable;
	QPainterPath ArrowHead;

	bool cancelTriggered;
	bool isOutside;
	QBrush mBrush;
	QPen mPen;
	QPointF lastPoint;
	bool panning;
	GridPluginToolBar* myToolbar;
	QCursor defaultCursor;
};



class GridPluginToolBar : public QToolBar {
	Q_OBJECT

public:

	enum {
		apply_icon = 0,
		cancel_icon,
		pan_icon,
		undo_icon,
		
		square_icon,
		rect_icon,

		icons_end,
	};


	GridPluginToolBar(const QString & title, QWidget * parent = 0);
	virtual ~GridPluginToolBar();

	void setPenColor(const QColor& col);
	void setPenWidth(int width);


public slots:
	void on_applyAction_triggered();
	void on_cancelAction_triggered();
	void on_panAction_toggled(bool checked);
	// void on_pencilAction_toggled(bool checked);
	// void on_lineAction_toggled(bool checked);
	// void on_arrowAction_toggled(bool checked);
	// void on_circleAction_toggled(bool checked);
	// void on_squarefillAction_toggled(bool checked);
	// void on_blurAction_toggled(bool checked);
	void on_squareAction_triggered(bool checked);
	// void on_textAction_toggled(bool checked);
	void on_penColButton_clicked();
	void on_widthBox_valueChanged(int val);
	void on_alphaBox_valueChanged(int val);
	void on_textInput_textChanged(const QString &text);
	void on_textInput_editingFinished();
	void on_undoAction_triggered();
	void showLineEdit(bool show);
	virtual void setVisible(bool visible);

signals:
	void applySignal();
	void cancelSignal();
	void colorSignal(QColor color);
	void widthSignal(int width);
	void paintHint(int paintMode);
	void shadingHint(bool invert);
	void panSignal(bool checked);
	void undoSignal();
	void modeChangeSignal(bool mode);
	void textChangeSignal(const QString &text);
	void editFinishSignal();

protected:
	void createLayout();
	void createIcons();

	QPushButton* penColButton;
	QColorDialog* colorDialog;
	QSpinBox* widthBox;
	QSpinBox* alphaBox;
	QColor penCol;
	int penAlpha;
	QMap<QString, QAction*> toolbarWidgetList;
	QAction* panAction;
	QAction* undoAction;

	QAction* pencilAction;
	QAction* lineAction;
	QAction* arrowAction;
	QAction* circleAction;
	QAction* squareAction;
	QAction* squarefillAction;
	QAction* blurAction;
	QAction* textAction;

	QLineEdit* textInput;

	QVector<QIcon> icons;		// needed for colorizing
	
};

};
