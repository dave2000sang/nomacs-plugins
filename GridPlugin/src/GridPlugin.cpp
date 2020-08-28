/*******************************************************************************************************
 GridPlugin.cpp

 nomacs is a fast and small image viewer with the capability of synchronizing multiple instances

 Copyright (C) 2015 #YOUR_NAME

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
#include "GridPlugin.h"
#include "DkToolbars.h"

#include <QDebug>
#include <QMouseEvent>

#include <iostream>


#pragma warning(push, 0)	// no warnings from includes - begin
#include <QAction>
#pragma warning(pop)		// no warnings from includes - end

namespace nmc {

// Adds diagonal, horizontal, and vertical grid lines (assuming rectangle exists from begin to point)
void GridPluginViewPort::addRiceGrid(QPointF begin, QPointF point) {
	paths.last().lineTo(point);
	paths.last().moveTo(QPoint(begin.x(), point.y()));
	paths.last().lineTo(QPoint(point.x(), begin.y()));
	// horizontal and vertical lines
	paths.last().moveTo(QPoint(begin.x(), begin.y() + (point.y() - begin.y())/2));
	paths.last().lineTo(QPoint(point.x(), begin.y() + (point.y() - begin.y())/2));
	paths.last().moveTo(QPoint(begin.x() + (point.x() - begin.x())/2, begin.y()));
	paths.last().lineTo(QPoint(begin.x() + (point.x() - begin.x())/2, point.y()));
}

/**
*	Constructor
**/
GridPlugin::GridPlugin(QObject* parent) : QObject(parent) {
	viewport = 0;
}

/**
*	Destructor
**/
GridPlugin::~GridPlugin() {}

/**
* Returns descriptive image for every ID
* @param plugin ID
**/
QImage GridPlugin::image() const {
	return QImage(":/gridPlugin/img/description.png");
};


bool GridPlugin::hideHUD() const {
	return true;
}

/**
* Main function: runs plugin based on its ID
* @param run ID
* @param current image in the Nomacs viewport
**/
const int ArrowWidth = 10;
const int ArrowHeight = 18;
const int TextEnlarge = 15;


/**
* Main function: runs plugin based on its ID
* @param plugin ID
* @param image to be processed
**/
QSharedPointer<nmc::DkImageContainer> GridPlugin::runPlugin(const QString &runID, QSharedPointer<nmc::DkImageContainer> image) const {
	Q_UNUSED(runID);

	if (!image)
		return image;

	//for a viewport plugin runID and image are null
	if (viewport) {

		GridPluginViewPort* myviewport = dynamic_cast<GridPluginViewPort*>(viewport);

		if (!myviewport->isCanceled()) 
			image->setImage(myviewport->getPaintedImage(), tr("Drawings Added"));

		viewport->setVisible(false);
		
	}
	
	return image;
};


/**
* returns paintViewPort
**/
nmc::DkPluginViewPort* GridPlugin::getViewPort() {
	return viewport;
}

GridPluginViewPort * GridPlugin::getGridPluginViewPort() {
	return dynamic_cast<GridPluginViewPort*>(viewport);
}

bool GridPlugin::createViewPort(QWidget * parent) {
	
	viewport = new GridPluginViewPort(parent);
	
	return true;
}

void GridPlugin::setVisible(bool visible) {

	if (!viewport)
		return;

	viewport->setVisible(visible);
	if (!visible)
		getGridPluginViewPort()->clear();
	
}


/*-----------------------------------GridPluginViewPort ---------------------------------------------*/


GridPluginViewPort::GridPluginViewPort(QWidget* parent, Qt::WindowFlags flags) : DkPluginViewPort(parent, flags) {

	setObjectName("GridPluginViewPort");
	init();
	setMouseTracking(true);
}

GridPluginViewPort::~GridPluginViewPort() {
	
	saveSettings();

	// active deletion since the MainWindow takes ownership...
	// if we have issues with this, we could disconnect all signals between viewport and toolbar too
	// however, then we have lot's of toolbars in memory if the user opens the plugin again and again
	if (myToolbar) {
		delete myToolbar;
		myToolbar = 0;
	}
}

void GridPluginViewPort::saveSettings() const {

	nmc::DefaultSettings settings;

	settings.beginGroup(objectName());
	settings.setValue("penColor", mPen.color().rgba());
	settings.setValue("penWidth", mPen.width());
	settings.endGroup();

}

void GridPluginViewPort::loadSettings() {

	nmc::DefaultSettings settings;

	settings.beginGroup(objectName());
	mPen.setColor(QColor::fromRgba(settings.value("penColor", mPen.color().rgba()).toInt()));
	mPen.setWidth(settings.value("penWidth", 1).toInt());
	settings.endGroup();

}

void GridPluginViewPort::init() {
	
	panning = false;
	cancelTriggered = false;
	isOutside = false;
	defaultCursor = Qt::CrossCursor;
	setCursor(defaultCursor);
	mPen = QColor(255,0,0);
	mPen.setCapStyle(Qt::RoundCap);
	mPen.setJoinStyle(Qt::RoundJoin);
	mPen.setWidth(1);
	QColor mPenColor = mPen.color();
	mPenColor.setAlpha(255/2 + 1);
	mPen.setColor(mPenColor);
	selectedMode = mode_rect;		// start with mode_rect

	myToolbar = new GridPluginToolBar(tr("Toolbar"), this);

	connect(myToolbar, SIGNAL(colorSignal(QColor)), this, SLOT(setPenColor(QColor)), Qt::UniqueConnection);
	connect(myToolbar, SIGNAL(widthSignal(int)), this, SLOT(setPenWidth(int)), Qt::UniqueConnection);
	connect(myToolbar, SIGNAL(panSignal(bool)), this, SLOT(setPanning(bool)), Qt::UniqueConnection);
	connect(myToolbar, SIGNAL(cancelSignal()), this, SLOT(discardChangesAndClose()), Qt::UniqueConnection);
	connect(myToolbar, SIGNAL(undoSignal()), this, SLOT(undoLastPaint()), Qt::UniqueConnection);
	connect(myToolbar, SIGNAL(modeChangeSignal(bool)), this, SLOT(toggleSquare(bool)), Qt::UniqueConnection);
	connect(myToolbar, SIGNAL(applySignal()), this, SLOT(applyChangesAndClose()), Qt::UniqueConnection);
	connect(myToolbar, SIGNAL(textChangeSignal(const QString&)), this, SLOT(textChange(const QString&)), Qt::UniqueConnection);
	connect(myToolbar, SIGNAL(editFinishSignal()), this, SLOT(textEditFinsh()), Qt::UniqueConnection);
	connect(this, SIGNAL(editShowSignal(bool)), myToolbar, SLOT(showLineEdit(bool)), Qt::UniqueConnection);
	
	loadSettings();
	myToolbar->setPenColor(mPen.color());
	myToolbar->setPenWidth(mPen.width());
	textinputenable = false;
}



void GridPluginViewPort::undoLastPaint() {

	if (paths.empty())
		return;		// nothing to undo

	paths.pop_back();
	pathsPen.pop_back();
	pathsMode.pop_back();
	update();
}

void GridPluginViewPort::mousePressEvent(QMouseEvent *event) {

	// panning -> redirect to viewport
	if (event->buttons() == Qt::LeftButton && 
		(event->modifiers() == nmc::DkSettingsManager::param().global().altMod || panning)) {
		setCursor(Qt::ClosedHandCursor);
		event->setModifiers(Qt::NoModifier);	// we want a 'normal' action in the viewport
		event->ignore();
		return;
	}

	if (event->buttons() == Qt::LeftButton && parent()) {

		nmc::DkBaseViewPort* viewport = dynamic_cast<nmc::DkBaseViewPort*>(parent());
		if(viewport) {
		
			if(QRectF(QPointF(), viewport->getImage().size()).contains(mapToImage(event->pos()))) {
					
				isOutside = false;

				// roll back the empty painterpath generated by click mouse
				if(!paths.empty())
					if(paths.last().isEmpty())
						undoLastPaint();

				// create new painterpath
				paths.append(QPainterPath());
				paths.last().moveTo(mapToImage(event->pos()));
				//paths.last().lineTo(mapToImage(event->pos())+QPointF(0.1,0));
				begin = mapToImage(event->pos());
				pathsPen.append(mPen);
				pathsMode.append(selectedMode);
				update();
			}
			else 
				isOutside = true;
		}
	}

	// no propagation
}

void GridPluginViewPort::mouseMoveEvent(QMouseEvent *event) {

	//qDebug() << "paint viewport...";

	// panning -> redirect to viewport
	if (event->modifiers() == nmc::DkSettingsManager::param().global().altMod ||
		panning) {

		event->setModifiers(Qt::NoModifier);
		event->ignore();
		update();
		return;
	}

	if (parent()) {
		nmc::DkBaseViewPort* viewport = dynamic_cast<nmc::DkBaseViewPort*>(parent());

		if (viewport) {
			viewport->unsetCursor();

			if (event->buttons() == Qt::LeftButton && parent()) {

				if (QRectF(QPointF(), viewport->getImage().size()).contains(mapToImage(event->pos()))) {
					if (isOutside) {
						paths.append(QPainterPath());
						paths.last().moveTo(mapToImage(event->pos()));
						pathsPen.append(mPen);
						pathsMode.append(selectedMode);
					}
					else {
						QPointF point = mapToImage(event->pos());
						float x, y, len;
						QPointF point_square;
						switch(selectedMode) {
							case mode_square:
								x = point.x();
								len = std::abs(x - begin.x());
								y = point.y() > begin.y() ? begin.y() + len : begin.y() - len;
								point_square = QPointF(x, y);
								paths.last() = QPainterPath();
								paths.last().addRect(QRectF(begin, point_square));
								paths.last().addEllipse(QRectF(begin, point_square));
								addRiceGrid(begin, point_square);
								break;
							case mode_rect:
								paths.last() = QPainterPath();
								paths.last().addRect(QRectF(begin, point));
								paths.last().addEllipse(QRectF(begin, point));
								addRiceGrid(begin, point);
								break;
						}
						update();
					}
					isOutside = false;
				}
				else 
					isOutside = true;
			}
		}
	}
	//QWidget::mouseMoveEvent(event);	// do not propagate mouse event
}

void GridPluginViewPort::mouseReleaseEvent(QMouseEvent *event) {

	// panning -> redirect to viewport
	if (event->modifiers() == nmc::DkSettingsManager::param().global().altMod || panning) {
		setCursor(defaultCursor);
		event->setModifiers(Qt::NoModifier);
		event->ignore();
		return;
	}
}

void GridPluginViewPort::paintEvent(QPaintEvent *event) {

	QPainter painter(this);
	
	if (mWorldMatrix)
		painter.setWorldTransform((*mImgMatrix) * (*mWorldMatrix));	// >DIR: using both matrices allows for correct resizing [16.10.2013 markus]

	for (int idx = 0; idx < paths.size(); idx++) {
		painter.setPen(pathsPen.at(idx));
		painter.drawPath(paths.at(idx));
	}

	painter.end();

	DkPluginViewPort::paintEvent(event);
}

QImage GridPluginViewPort::getPaintedImage() {

	if(parent()) {
		nmc::DkBaseViewPort* viewport = dynamic_cast<nmc::DkBaseViewPort*>(parent());
		if (viewport) {

			if (!paths.isEmpty()) {   // if nothing is drawn there is no need to change the image

				QImage img = viewport->getImage();

				QPainter painter(&img);

				// >DIR: do not apply world matrix if painting in the image [14.10.2014 markus]
				//if (worldMatrix)
				//	painter.setWorldTransform(*worldMatrix);

				painter.setRenderHint(QPainter::HighQualityAntialiasing);
				painter.setRenderHint(QPainter::Antialiasing);

				for (int idx = 0; idx < paths.size(); idx++) {
					painter.setPen(pathsPen.at(idx));
					painter.drawPath(paths.at(idx));
				}
				painter.end();

				return img;
			}
		}
	}
	
	return QImage();
}


void GridPluginViewPort::toggleSquare(bool checked){
	if (checked) {
		selectedMode = mode_square;
	} else {
		selectedMode = mode_rect;
	}
	setCursor(defaultCursor);
	emit editShowSignal(false);
  
	this->repaint();
}

void GridPluginViewPort::textChange(const QString &text){
	QFont font;
	font.setFamily(font.defaultFamily());
	font.setPixelSize(mPen.width()*TextEnlarge);
	if(textinputenable)
	{
		sbuffer = text;
		paths.last() = QPainterPath();
		paths.last().addText(begin, font, text);
		update();
	}
}

void GridPluginViewPort::textEditFinsh(){
	if(sbuffer.isEmpty())
		undoLastPaint();
	textinputenable = false;
	emit editShowSignal(false);
}

void GridPluginViewPort::clear() {
	paths.clear();
	pathsPen.clear();
	pathsMode.clear();
}

void GridPluginViewPort::setBrush(const QBrush& brush) {
	this->mBrush = brush;
}

void GridPluginViewPort::setPen(const QPen& pen) {
	this->mPen = pen;
}

void GridPluginViewPort::setPenWidth(int width) {

	this->mPen.setWidth(width);
}

void GridPluginViewPort::setPenColor(QColor color) {

	this->mPen.setColor(color);
}

void GridPluginViewPort::setPanning(bool checked) {

	this->panning = checked;
	if(checked) 
		defaultCursor = Qt::OpenHandCursor;
	else 
		defaultCursor = Qt::CrossCursor;
	setCursor(defaultCursor);
}

void GridPluginViewPort::applyChangesAndClose() {

	cancelTriggered = false;
	emit closePlugin();
}

void GridPluginViewPort::discardChangesAndClose() {

	cancelTriggered = true;
	emit closePlugin();
}

QBrush GridPluginViewPort::getBrush() const {
	return mBrush;
}

QPen GridPluginViewPort::getPen() const {
	return mPen;
}

bool GridPluginViewPort::isCanceled() {
	return cancelTriggered;
}

void GridPluginViewPort::setVisible(bool visible) {

	if (myToolbar)
		nmc::DkToolBarManager::inst().showToolBar(myToolbar, visible);

	DkPluginViewPort::setVisible(visible);
}








/*-----------------------------------GridPluginToolBar ---------------------------------------------*/
GridPluginToolBar::GridPluginToolBar(const QString & title, QWidget * parent /* = 0 */) : QToolBar(title, parent) {

	setObjectName("GridPluginToolBar");
	createIcons();
	createLayout();
	QMetaObject::connectSlotsByName(this);

	qDebug() << "[GridPlugin TOOLBAR] created...";
}

GridPluginToolBar::~GridPluginToolBar() {

	qDebug() << "[GridPlugin TOOLBAR] deleted...";
}

void GridPluginToolBar::createIcons() {

	// create icons
	icons.resize(icons_end);

	icons[apply_icon]	= nmc::DkImage::loadIcon(":/nomacs/img/save.svg");
	icons[cancel_icon]	= nmc::DkImage::loadIcon(":/nomacs/img/close.svg");
	icons[pan_icon]		= nmc::DkImage::loadIcon(":/nomacs/img/pan.svg");
	icons[pan_icon].addPixmap(nmc::DkImage::loadIcon(":/nomacs/img/pan-checked.svg"), QIcon::Normal, QIcon::On);
	icons[undo_icon]	= nmc::DkImage::loadIcon(":/nomacs/img/rotate-cc.svg");
	icons[square_icon] = nmc::DkImage::loadIcon(":/gridPlugin/img/square-outline.svg");
	// icons[rect_icon] = nmc::DkImage::loadIcon(":/gridPlugin/img/rect-outline.svg");
}

void GridPluginToolBar::createLayout() {

	QList<QKeySequence> enterSc;
	enterSc.append(QKeySequence(Qt::Key_Enter));
	enterSc.append(QKeySequence(Qt::Key_Return));

	QAction* applyAction = new QAction(icons[apply_icon], tr("Apply (ENTER)"), this);
	applyAction->setShortcuts(enterSc);
	applyAction->setObjectName("applyAction");

	QAction* cancelAction = new QAction(icons[cancel_icon], tr("Cancel (ESC)"), this);
	cancelAction->setShortcut(QKeySequence(Qt::Key_Escape));
	cancelAction->setObjectName("cancelAction");

	panAction = new QAction(icons[pan_icon], tr("Pan"), this);
	panAction->setShortcut(QKeySequence(Qt::Key_P));
	panAction->setObjectName("panAction");
	panAction->setCheckable(true);
	panAction->setChecked(false);

	// for square rice grid lines
	squareAction = new QAction(icons[square_icon], tr("Square"), this);
	squareAction->setObjectName("squareAction");
	squareAction->setCheckable(true);
	squareAction->setChecked(false);

	textInput = new QLineEdit(this);
	textInput->setObjectName("textInput");
	textInput->setFixedWidth(100);

	// mPen color
	penCol = QColor(0,0,0);
	penColButton = new QPushButton(this);
	penColButton->setObjectName("penColButton");
	penColButton->setStyleSheet("QPushButton {background-color: " + nmc::DkUtils::colorToString(penCol) + "; border: 1px solid #888;}");
	penColButton->setToolTip(tr("Background Color"));
	penColButton->setStatusTip(penColButton->toolTip());

	// undo Button
	undoAction = new QAction(icons[undo_icon], tr("Undo (CTRL+Z)"), this);
	undoAction->setShortcut(QKeySequence::Undo);
	undoAction->setObjectName("undoAction");

	colorDialog = new QColorDialog(this);
	colorDialog->setObjectName("colorDialog");

	// mPen width
	widthBox = new QSpinBox(this);
	widthBox->setObjectName("widthBox");
	widthBox->setSuffix("px");
	widthBox->setMinimum(1);
	widthBox->setMaximum(500);	// huge sizes since images might have high resolutions

	// mPen alpha
	alphaBox = new QSpinBox(this);
	alphaBox->setObjectName("alphaBox");
	alphaBox->setSuffix("%");
	alphaBox->setMinimum(0);
	alphaBox->setMaximum(100);

	// Cannot use QActionGroup because we want the squareAction to be toggled on/off
	// QActionGroup *modesGroup = new QActionGroup(this);
	// modesGroup->addAction(squareAction);

	toolbarWidgetList = QMap<QString, QAction*>();

	addAction(applyAction);
	addAction(cancelAction);
	addSeparator();
	addAction(panAction);
	addAction(undoAction);
	addSeparator();
	addAction(squareAction);
	addSeparator();
	addWidget(widthBox);
	addWidget(penColButton);
	addWidget(alphaBox);
	addSeparator();
	//addWidget(textInput);
	toolbarWidgetList.insert(textInput->objectName(), this->addWidget(textInput));

	showLineEdit(false);
}

void GridPluginToolBar::showLineEdit(bool show) {
	if(show)
	{
		toolbarWidgetList.value(textInput->objectName())->setVisible(true);
		textInput->setFocus();
	}
	else
		toolbarWidgetList.value(textInput->objectName())->setVisible(false);
}

void GridPluginToolBar::setVisible(bool visible) {

	//if (!visible)
	//	emit colorSignal(QColor(0,0,0));
	if (visible) {
		//emit colorSignal(penCol);
		//widthBox->setValue(10);
		//alphaBox->setValue(100);
		panAction->setChecked(false);
	}

	qDebug() << "[GridPlugin TOOLBAR] set visible: " << visible;

	QToolBar::setVisible(visible);
}

void GridPluginToolBar::setPenColor(const QColor& col) {

	penCol = col;
	penColButton->setStyleSheet("QPushButton {background-color: " + nmc::DkUtils::colorToString(penCol) + "; border: 1px solid #888;}");
	penAlpha = col.alpha();
	alphaBox->setValue(col.alphaF()*100);
}

void GridPluginToolBar::setPenWidth(int width) {

	widthBox->setValue(width);
}

void GridPluginToolBar::on_undoAction_triggered() {
	emit undoSignal();
}

void GridPluginToolBar::on_applyAction_triggered() {
	emit applySignal();
}

void GridPluginToolBar::on_cancelAction_triggered() {
	emit cancelSignal();
}

void GridPluginToolBar::on_panAction_toggled(bool checked) {
	Q_UNUSED(checked);
	emit panSignal(checked);
}

void GridPluginToolBar::on_squareAction_triggered(bool checked){
	Q_UNUSED(checked);
	emit modeChangeSignal(checked);
}

void GridPluginToolBar::on_widthBox_valueChanged(int val) {
	emit widthSignal(val);
}

void GridPluginToolBar::on_textInput_textChanged(const QString &text){
	emit textChangeSignal(text);
}

void GridPluginToolBar::on_textInput_editingFinished(){
	emit editFinishSignal();
	textInput->clear();
}

void GridPluginToolBar::on_alphaBox_valueChanged(int val) {

	penAlpha = val;
	QColor penColWA = penCol;
	penColWA.setAlphaF(penAlpha/100.0);
	emit colorSignal(penColWA);
}

void GridPluginToolBar::on_penColButton_clicked() {

	QColor tmpCol = penCol;
	
	colorDialog->setCurrentColor(tmpCol);
	int ok = colorDialog->exec();

	if (ok == QDialog::Accepted) {
		penCol = colorDialog->currentColor();
		penColButton->setStyleSheet("QPushButton {background-color: " + nmc::DkUtils::colorToString(penCol) + "; border: 1px solid #888;}");
		
		QColor penColWA = penCol;
		penColWA.setAlphaF(penAlpha/100.0);
		emit colorSignal(penColWA);
	}

}

};

