//
// Created by Sadeep on 11-Jun.
//

#include <Document.h>
#include<Display.h>
#include <brlcad/Database/Torus.h>


Document::Document(const int documentId, const QString *filePath) : documentId(documentId) {
    if (filePath != nullptr) this->filePath = new QString(*filePath);
    database =  new BRLCAD::MemoryDatabase();
    if (filePath != nullptr) {
        if (!database->Load(filePath->toUtf8().data()))
        {
            throw std::runtime_error("Failed to open file");
        }
    }

    modified = false;
    objectTree = new ObjectTree(database);
    properties = new Properties(*this);
    geometryRenderer = new GeometryRenderer(this);
    objectTreeWidget = new ObjectTreeWidget(this);
    displayGrid = new DisplayGrid(this);

    displayGrid->forceRerenderAllDisplays();

    raytraceWidget = new RaytraceView(this);
}

Document::~Document() {
    delete database;
}

void Document::modifyObject(BRLCAD::Object *newObject) {
    modified = true;
    database->Set(*newObject);
    QString objectName = newObject->Name();
    getObjectTree()->traverseSubTree(0,false,[this, objectName]
    (int objectId){
        if (getObjectTree()->getNameMap()[objectId] == objectName){
            geometryRenderer->clearObject(objectId);
        }
        return true;
    }
    );
    geometryRenderer->refreshForVisibilityAndSolidChanges();
    for (Display * display : displayGrid->getDisplays())display->forceRerenderFrame();
}

bool Document::isModified() {
    return modified;
}

bool Document::Add(const BRLCAD::Object& object) {
    modified = true;
    return database->Add(object);
}

bool Document::Save(const char* fileName) {
    modified = false;
    return database->Save(fileName);
}

class BRLCADConstObjectCallback {
public:
    BRLCADConstObjectCallback(const std::function<void(const BRLCAD::Object&)>& func): m_func(func) {}

    virtual void operator()(const BRLCAD::Object& object) {
        m_func(object);
    }

private:
    const std::function<void(const BRLCAD::Object&)>& m_func;
};

void Document::getBRLCADConstObject(const QString& objectName, const std::function<void(const BRLCAD::Object&)>& func) {
    BRLCADConstObjectCallback callback(func);
    database->Get(objectName.toUtf8(), callback);
}

class BRLCADObjectCallback {
public:
    BRLCADObjectCallback(const std::function<void(BRLCAD::Object&)>& func): m_func(func) {}

    virtual void operator()(BRLCAD::Object& object) {
        m_func(object);
    }

private:
    const std::function<void(BRLCAD::Object&)>& m_func;
};

void Document::getBRLCADObject(const QString& objectName, const std::function<void(BRLCAD::Object&)>& func) {
    BRLCADObjectCallback callback(func);
    database->Get(objectName.toUtf8(), callback);
    modified = true;
}

Display* Document::getDisplay()
{
    return displayGrid->getActiveDisplay();
}
