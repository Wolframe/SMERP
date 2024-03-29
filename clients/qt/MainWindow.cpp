//
// MainWindow.cpp
//

#include "MainWindow.hpp"
#include "FileFormLoader.hpp"

#include <QtUiTools>
#include <QtGui>
#include <QBuffer>

namespace _SMERP {
	namespace QtClient {

MainWindow::MainWindow( QWidget *_parent ) : QWidget( _parent ), m_ui( 0 ), m_form( 0 )
{
	m_formLoader = new FileFormLoader( "forms" );
	initialize( );
}

MainWindow::~MainWindow( )
{
	delete m_debugTerminal;
	delete m_smerpClient;
	delete m_formLoader;
}

void MainWindow::initialize( )
{
// load default theme
	loadTheme( QString( QLatin1String( "windows" ) ) );

// create a SMERP protocol client
	m_smerpClient = new SMERPClient( );

// create debuging terminal
	m_debugTerminal = new DebugTerminal( m_smerpClient, this );
}

void MainWindow::populateThemesMenu( )
{
// construct a menu which shows all available themes in a directory
	QDir themes_dir( QLatin1String( "themes" ) );
	QStringList themes = themes_dir.entryList( QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name );
	QMenu *themes_menu = qFindChild<QMenu *>( m_ui, "menuThemes" );
	QActionGroup *themesGroup = new QActionGroup( themes_menu );
	themesGroup->setExclusive( true );
	foreach( QString t, themes ) {
		QAction *action = new QAction( t, themesGroup );
		action->setCheckable( true );
		themesGroup->addAction( action );
		if( t == m_currentTheme ) action->setChecked( true );
	}

// connect signal for theme selection
	themes_menu->addActions( themesGroup->actions( ) );
	QObject::connect( themesGroup, SIGNAL( triggered( QAction * ) ), this, SLOT( themeSelected( QAction * ) ) );
}

void MainWindow::loadTheme( QString theme )
{
// indicate busy state
	qApp->setOverrideCursor( Qt::BusyCursor );

// set working directory to theme
	QString themesFolder( QLatin1String( "themes/" ) + theme + QLatin1Char( '/' ) );

// tell the loader that this is the working directory
	QUiLoader loader;
	loader.setWorkingDirectory( themesFolder );

// remember current user interface
	QWidget *oldUi = m_ui;

// load the main window (which is empty) and provides basic functions like
// theme switching, login, exit, about, etc. (to start unauthenticated)
	QFile file( themesFolder + QLatin1String( "MainWindow.ui" ) );
	file.open( QFile::ReadOnly );
	m_ui = loader.load( &file, this );
	file.close( );

// set stylesheet of the application (has impact on the whole application)
	QFile qss( themesFolder + QLatin1String( "MainWindow.qss" ) );
	qApp->setStyleSheet( QLatin1String( "file:///" ) + QFileInfo( qss ).absoluteFilePath( ) );

// wire standard actions in the menu by name (on_<object>_<signal>)
// (true autowiring doesn't work: from now signals to ghost signals everything
// happens)
	QAction *actionExit = qFindChild<QAction *>( m_ui, "actionExit" );
	QObject::connect( actionExit, SIGNAL( triggered( ) ), this, SLOT( on_actionExit_triggered( ) ) );

	QAction *actionAbout = qFindChild<QAction *>( m_ui, "actionAbout" );
	QObject::connect( actionAbout, SIGNAL( triggered( ) ), this, SLOT( on_actionAbout_triggered( ) ) );

	QAction *actionAboutQt = qFindChild<QAction *>( m_ui, "actionAboutQt" );
	QObject::connect( actionAboutQt, SIGNAL( triggered( ) ), this, SLOT( on_actionAboutQt_triggered( ) ) );

	QAction *actionDebugTerminal = qFindChild<QAction *>( m_ui, "actionDebugTerminal" );
	QObject::connect( actionDebugTerminal, SIGNAL( triggered( bool ) ), this, SLOT( on_actionDebugTerminal_triggered( bool ) ) ); 

// copy over the location of the old window to the new one
// also copy over the current form, don't destroy the old ui,
// events could be outstanding (deleteLater marks the widget 
// for deletion, will be deleted when returning into the event
// loop)
	if( oldUi ) {
		m_ui->move( oldUi->pos( ) );
		oldUi->hide( );
		oldUi->deleteLater( );
	}

// show the new gui
	m_ui->show( );
	
// remember current theme
	m_currentTheme = theme;

// load all themes possible to pick and mark the current one
	populateThemesMenu( );

// now that we have a menu where we can add things, we start the form list loading
	QObject::connect( m_formLoader, SIGNAL( formListLoaded( ) ), this, SLOT( formListLoaded( ) ) );
	m_formLoader->initiateListLoad( );
	
// not busy anymore
	qApp->restoreOverrideCursor( );

// load the current form again
	if( m_form ) loadForm( m_currentForm );
}

void MainWindow::formListLoaded( )
{
// get the list of available forms
	QStringList forms = m_formLoader->getFormNames( );

// contruct a menu which shows and wires them in the menu
	QMenu *formsMenu = qFindChild<QMenu *>( m_ui, "menuForms" );
	formsMenu->clear( );
	QActionGroup *formGroup = new QActionGroup( formsMenu );
	formGroup->setExclusive( true );
	foreach( QString form, forms ) {
		QAction *action = new QAction( form, formGroup );
		action->setCheckable( true );
		formGroup->addAction( action );
		if( form == m_currentForm ) action->setChecked( true );
	}
	formsMenu->addActions( formGroup->actions( ) );
	QObject::connect( formGroup, SIGNAL( triggered( QAction * ) ), this, SLOT( formSelected( QAction * ) ) );
	
// not busy anymore
	qApp->restoreOverrideCursor();
}

void MainWindow::formSelected( QAction *action )
{
	QString form = action->text( );
	if( form != m_currentForm )
		loadForm( form );
}

void MainWindow::loadForm( QString form )
{
// indicate busy state
	qApp->setOverrideCursor( Qt::BusyCursor );

	QObject::connect( m_formLoader, SIGNAL( formLoaded( QString, QByteArray ) ),
		this, SLOT( formLoaded( QString, QByteArray ) ) );
	
	m_formLoader->initiateFormLoad( form );
}

void MainWindow::formLoaded( QString name, QByteArray xml )
{
// read the form and construct it
	QWidget *oldForm = m_form;
	QUiLoader loader;
	QBuffer buf( &xml );
	m_form = loader.load( &buf, m_ui );
	buf.close( );

// add it to the main window, disable old form
	QVBoxLayout *l = qFindChild<QVBoxLayout *>( m_ui, "mainAreaLayout" );
	l->addWidget( m_form );

	if( oldForm ) {
		m_form->move( oldForm->pos( ) );
		oldForm->hide( );
		oldForm->deleteLater( );
		oldForm = NULL;
	}
	m_form->show( );

// remember the name of the current form
	m_currentForm = name;
	
// not busy anymore
	qApp->restoreOverrideCursor();
}

void MainWindow::themeSelected( QAction *action )
{
	QString theme = action->text( );
	if( theme != m_currentTheme )
		loadTheme( theme );
}

void MainWindow::on_actionExit_triggered( )
{
	close( );
}

void MainWindow::on_actionAbout_triggered( )
{
	QString info = QString( tr( "qtclient" ) );
	QMessageBox::about( m_ui, tr( "qtclient" ), info );
}

void MainWindow::on_actionAboutQt_triggered( )
{
	QMessageBox::aboutQt( m_ui, tr( "qtclient" ) );
}

void MainWindow::on_actionDebugTerminal_triggered( bool checked )
{
	if( checked ) {
		m_debugTerminal->bringToFront( );
	} else {
		m_debugTerminal->hide( );
	}
}

} // namespace QtClient
} // namespace _SMERP
