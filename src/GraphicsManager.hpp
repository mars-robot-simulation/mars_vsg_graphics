#pragma once

#include "gui_helper_functions.hpp"

#include <mars_interfaces/graphics/GraphicsManagerInterface.hpp>
#include <mars_interfaces/graphics/GraphicsEventInterface.h>

#include <cfg_manager/CFGManagerInterface.h>
#include <cfg_manager/CFGClient.h>

// DataBroker includes
#include <data_broker/ProducerInterface.h>
#include <data_broker/DataBrokerInterface.h>
#include <data_broker/DataPackageMapping.h>

#include <vsg/all.h>
#include <vsgXchange/all.h>
#include <vsgQt/Window.h>

namespace mars
{
    namespace vsg_graphics
    {
        class DrawObject;
        class GuiHelper;

        class GraphicsManager : public interfaces::GraphicsManagerInterface,
                                public interfaces::GraphicsEventInterface,
                                public cfg_manager::CFGClient,
                                public data_broker::ProducerInterface

        {

        public:
            GraphicsManager(lib_manager::LibManager *theManager, void *QTWidget = 0);
            ~GraphicsManager();

            CREATE_MODULE_INFO();


            virtual void cfgUpdateProperty(cfg_manager::cfgPropertyStruct _property) override;

            virtual void produceData(const data_broker::DataInfo &info,
                                     data_broker::DataPackage *dbPackage,
                                     int callbackParam) override;

            virtual void* getWindowManager(int id=1) override; // get osgWidget WindowManager*

            int getLibVersion() const override
                { return 1; }
            const std::string getLibName() const override
                { return std::string("mars_graphics"); }

            virtual void addDrawItems(interfaces::drawStruct *draw) override; ///< Adds \c drawStruct items to the graphics scene
            virtual void removeDrawItems(interfaces::DrawInterface *iface) override;
            virtual void clearDrawItems(void) override;

            virtual void addLight(interfaces::LightData &ls) override; ///< Adds a light to the scene.

            virtual void addGraphicsUpdateInterface(interfaces::GraphicsUpdateInterface *g) override;
            virtual void removeGraphicsUpdateInterface(interfaces::GraphicsUpdateInterface *g) override;
            virtual unsigned long addDrawObject(const interfaces::NodeData &snode,
                                                bool activated = true) override;
            virtual unsigned long getDrawID(const std::string &name) const override;
            virtual void removeDrawObject(unsigned long id) override;
            virtual void setDrawObjectPos(unsigned long id,
                                          const mars::utils::Vector &pos) override;
            virtual void setDrawObjectRot(unsigned long id,
                                          const mars::utils::Quaternion &q) override;
            virtual void setDrawObjectScale(unsigned long id,
                                            const mars::utils::Vector &scale) override;
            virtual void setDrawObjectScaledSize(unsigned long id,
                                                 const mars::utils::Vector &ext) override;
            virtual void setDrawObjectMaterial(unsigned long id,
                                               const interfaces::MaterialData &material) override;
            virtual void addMaterial(const interfaces::MaterialData &material) override;
            virtual void setDrawObjectNodeMask(unsigned long id, unsigned int bits) override;

            virtual void closeAxis() override; ///< Closes existing joint axis.

            virtual void drawAxis(const mars::utils::Vector &first,
                                  const mars::utils::Vector &second,
                                  const mars::utils::Vector &third,
                                  const mars::utils::Vector &axis1,
                                  const mars::utils::Vector &axis2) override; ///< Draws 2 axes from first to second to third and 2 joint axes in the widget.

            virtual void getCameraInfo(interfaces::cameraStruct *cs) const override;  ///< Returns current camera information.

            virtual void* getStateSet() const override; ///< Provides access to global state set.

            virtual void* getScene() const override; ///< Provides access to the graphics scene.
            virtual void* getScene2() const override; ///< ?

            virtual void hideCoords() override; ///< Removes the main coordination frame from the scene.

            virtual void hideCoords(const mars::utils::Vector &pos) override; ///< Removes current coordination frame from the scene.

            virtual void showClouds() override;
            virtual void hideClouds() override;

            virtual void preview(int action, bool resize,
                                 const std::vector<interfaces::NodeData> &allNodes,
                                 unsigned int num = 0,
                                 const interfaces::MaterialData *mat = 0) override; ///< Creates a preview node.

            virtual void removeLight(unsigned int index) override; ///< Removes a light from the scene.

            virtual void removePreviewNode(unsigned long id) override; ///< Removes a preview node.

            virtual void reset() override; ///< Resets scene.

            virtual void setCamera(int type) override; ///< Sets the camera type.

            virtual void showCoords() override; ///< Adds the main coordination frame to the scene.

            virtual void showCoords(const mars::utils::Vector &pos,
                                    const mars::utils::Quaternion &rot,
                                    const mars::utils::Vector &size) override; ///< Adds a local coordination frame to the scene.

            virtual bool coordsVisible(void) const override;
            virtual bool gridVisible(void) const override;
            virtual bool cloudsVisible(void) const override;

            virtual void update() override; ///< Update the scene.
            virtual void saveScene(const std::string &filename) const override;
            virtual const interfaces::GraphicData getGraphicOptions(void) const override;
            virtual void setGraphicOptions(const interfaces::GraphicData &options,
                                           bool ignoreClearColor=false) override;
            virtual void showGrid(void) override;
            virtual void hideGrid(void) override;
            virtual void updateLight(unsigned int index, bool recompileShader=false) override;
            virtual void getLights(std::vector<interfaces::LightData*> *lightList) override;
            virtual void getLights(std::vector<interfaces::LightData> *lightList) const override;
            virtual int getLightCount(void) const override;
            virtual void exportScene(const std::string &filename) const override;
            virtual void setTexture(unsigned long id, const std::string &filename) override;
            virtual unsigned long new3DWindow(void *myQTWidget = 0, bool rtt = 0,
                                              int width = 0, int height = 0, const std::string &name = std::string("")) override;
            virtual void setGrabFrames(bool value) override;
            virtual interfaces::GraphicsWindowInterface* get3DWindow(unsigned long id) const override;
            virtual interfaces::GraphicsWindowInterface* get3DWindow(const std::string &name) const override;
            virtual void remove3DWindow(unsigned long id) override;
            virtual void getList3DWindowIDs(std::vector<unsigned long> *ids) const override;
            virtual void removeLayerFromDrawObjects(unsigned long window_id) override;

            // HUD Interface:
            virtual unsigned long addHUDElement(interfaces::hudElementStruct *new_hud_element) override;
            virtual void removeHUDElement(unsigned long id) override;
            virtual void switchHUDElementVis(unsigned long id) override;
            virtual void setHUDElementPos(unsigned long id, double x, double y) override;
            virtual void setHUDElementTextureData(unsigned long id, void* data) override;
            virtual void setHUDElementTextureRTT(unsigned long id,
                                                 unsigned long window_id,
                                                 bool depthComponent = false) override;
            virtual void setHUDElementTexture(unsigned long id,
                                              std::string texturename) override;
            virtual void setHUDElementLabel(unsigned long id, std::string text,
                                            double text_color[4]) override;
            virtual void setHUDElementLines(unsigned long id, std::vector<double> *v,
                                            double color[4]) override;
            virtual void* getQTWidget(unsigned long id) const override;
            virtual void showQTWidget(unsigned long id) override;
            virtual void addGuiEventHandler(interfaces::GuiEventInterface *_guiEventHandler) override;
            virtual void removeGuiEventHandler(interfaces::GuiEventInterface *_guiEventHandler) override;
            virtual void exportDrawObject(unsigned long id,
                                          const std::string &name) const override;
            virtual void setBlending(unsigned long id, bool mode) override;
            virtual void setBumpMap(unsigned long id, const std::string &bumpMap) override;
            virtual void setGraphicsWindowGeometry(unsigned long id, int top,
                                                   int left, int width, int height) override;
            virtual void getGraphicsWindowGeometry(unsigned long id,
                                                   int *top, int *left,
                                                   int *width, int *height) const override;
            virtual void setActiveWindow(unsigned long win_id) override;
            virtual void setDrawObjectSelected(unsigned long id, bool val) override;
            virtual void setDrawObjectShow(unsigned long id, bool val) override;
            virtual void setDrawObjectRBN(unsigned long id, int val) override;
            virtual void addEventClient(interfaces::GraphicsEventClient* theClient) override;
            virtual void removeEventClient(interfaces::GraphicsEventClient* theClient) override;
            virtual void setSelectable(unsigned long id, bool val) override;
            virtual void showNormals(bool val) override;
            virtual void* getView(unsigned long id=1) override; ///< Returns the view of a window. The first window has id 1, this is also the default value. Return 0 if the window does not exist.
            virtual void collideSphere(unsigned long id, mars::utils::Vector pos,
                                       interfaces::sReal radius) override;
            virtual const utils::Vector& getDrawObjectPosition(unsigned long id=0) override;
            virtual const utils::Quaternion& getDrawObjectQuaternion(unsigned long id=0) override;

            virtual void draw() override;
            virtual void lock() override;
            virtual void unlock() override;

            virtual void initializeOSG(void *data, bool createWindow=true) override;
            virtual interfaces::LoadMeshInterface* getLoadMeshInterface(void) override;
            virtual interfaces::LoadHeightmapInterface* getLoadHeightmapInterface(void) override;

            virtual void makeChild(unsigned long parentId, unsigned long childId) override;
            virtual void attacheCamToNode(unsigned long winID, unsigned long drawID) override;

            virtual void setExperimentalLineLaser(utils::Vector pos, utils::Vector normal, utils::Vector color, utils::Vector laserAngle, float openingAngle) override;
            virtual void deactivate3DWindow(unsigned long id) override;
            virtual void activate3DWindow(unsigned long id) override;

            // be carful with this method, only add a valid pointer osg::Node*
            virtual void addOSGNode(void* node) override;
            virtual void removeOSGNode(void* node) override;
            virtual unsigned long addHUDOSGNode(void* node) override;
            virtual bool isInitialized() const override;
            virtual std::vector<interfaces::MaterialData> getMaterialList() const override;
            virtual void editMaterial(std::string materialName, std::string key,
                                      std::string value) override;
            /**
             * Applies the predefined default views on the 'active' view.
             * view:
             *       1: top
             *       2: front
             *       3: right
             *       4: back
             *       5: left
             *       6: down
             */
            virtual void setCameraDefaultView(int view) override;
            virtual void setDrawObjectBrightness(unsigned long id, double v) override;
            virtual void editLight(unsigned long id, const std::string &key,
                                   const std::string &value) override;
            virtual void edit(const std::string &key, const std::string &value) override;
            virtual void edit(unsigned long widgetID, const std::string &key,
                              const std::string &value) override;
            virtual void brushTest(mars::utils::Vector start, mars::utils::Vector end) override;
            virtual void brushTestThreaded(std::vector<utils::Vector> start_, std::vector<utils::Vector> end) override;

        private:
            interfaces::GraphicData graphicOptions;
            vsg::ref_ptr<vsgQt::Viewer> viewer;
            vsgQt::Window *window;
            QWidget *container;
            vsg::ref_ptr<vsg::Group> rootNode;
            vsg::ref_ptr<vsg::Node> coords;
            unsigned long long nextDrawID;
            std::map<unsigned long long, DrawObject*> drawObjects;
            GuiHelper *guiHelper;
            vsg::ref_ptr<WorldTransformUniformValue> worldTransformUniform;

            // mars event handling
            std::list<interfaces::GraphicsUpdateInterface*> graphicsUpdateObjects;

            // cfg_manager stuff
            cfg_manager::CFGManagerInterface *cfg;
            cfg_manager::cfgPropertyStruct resourcesPath, showCoords_;
            std::vector<cfg_manager::cfgPropertyStruct*> cfgProperties;
            void setupCFG(void);

        }; // end of class GraphicsManagerInterface

    } // end of namespace interfaces
} // end of namespace mars
