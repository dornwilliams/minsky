/*
  @copyright Steve Keen 2012
  @author Russell Standish
  This file is part of Minsky.

  Minsky is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Minsky is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Minsky.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MINSKY_H
#define MINSKY_H

#include "intrusiveMap.h"
#include "selection.h"
#include "godleyIcon.h"
#include "operation.h"
#include "evalOp.h"
#include "evalGodley.h"
#include "wire.h"
#include "plotWidget.h"
#include "version.h"
#include "variable.h"
#include "equations.h"
#include "latexMarkup.h"
#include "integral.h"
#include "variableValue.h"

#include <vector>
#include <string>
#include <set>
#include <deque>
using namespace std;

#include <ecolab.h>
#include <xml_pack_base.h>
#include <xml_unpack_base.h>
using namespace ecolab;
using namespace classdesc;

namespace minsky
{
  using namespace std;
  using classdesc::shared_ptr;

  struct RKdata; // an internal structure for holding Runge-Kutta data

  // a place to put working variables of the Minsky class that needn't
  // be serialised.
  struct MinskyExclude
  {
    EvalOpVector equations;
    vector<Integral> integrals;
    shared_ptr<RKdata> ode;
    shared_ptr<ofstream> outputDataFile;

    enum StateFlags {is_edited=1, reset_needed=2};
    int flags=reset_needed;
    
    std::vector<int> flagStack;

    // make copy operations just dummies, as assignment of Minsky's
    // doesn't need to change this
    MinskyExclude(): historyPtr(0) {}
    MinskyExclude(const MinskyExclude&): historyPtr(0) {}
    MinskyExclude& operator=(const MinskyExclude&) {return *this;}
  protected:
    /// save history of model for undo
    /* 
       TODO: it should be sufficient to add move semantics to pack_t,
       but for some reason copy semantics are required, requiring the
       use of shared_ptr
     */
    std::deque<classdesc::pack_t> history;
    size_t historyPtr;
  };

  /// convenience class for accessing matrix elements from a data array
  class MinskyMatrix
  {
    size_t n;
    double *data;
    CLASSDESC_ACCESS(MinskyMatrix);
  public:
    MinskyMatrix(size_t n, double* data): n(n), data(data) {}
    double& operator()(size_t i, size_t j) {return data[i*n+j];}
    double operator()(size_t i, size_t j) const {return data[i*n+j];}
  };

  enum ItemType {wire, op, var, group, godley, plot};

  class Minsky: public ValueVector, public Exclude<MinskyExclude>
  {
    CLASSDESC_ACCESS(Minsky);

    /// returns a diagnostic about an item that is infinite or
    /// NaN. Either a variable name, or and operator type.
    std::string diagnoseNonFinite() const;

    /// write current state of all variables to the log file
    void logVariables() const;

  protected:
    /// contents of current selection
    Selection currentSelection;

  public:

    /// reflects whether the model has been changed since last save
    bool edited() const {return flags & is_edited;}
    /// true if reset needs to be called prior to numerical integration
    bool reset_flag() const {return flags & reset_needed;}
    /// indicate model has been changed since last saved
    void markEdited() {flags |= is_edited | reset_needed;}

    /// @{ push and pop state of the flags
    void pushFlags() {flagStack.push_back(flags);}
    void popFlags() {
      if (!flagStack.empty()) {
        flags=flagStack.back();
        flagStack.pop_back();
      }
    }
    /// @}
    
    VariableValues variableValues;

    void setGodleyIconResource(const string& s)
    {GodleyIcon::svgRenderer.setResource(s);}
    void setGroupIconResource(const string& s)
    {Group::svgRenderer.setResource(s);}

    /// @return available matching columns from other Godley tables
    /// @param currTable - this table, not included in the matching process
    //  @param ac type of column we wish matches for
    std::set<string> matchingTableColumns(GodleyTable& currTable, GodleyAssetClass::AssetClass ac);

    /// find any duplicate column, and use it as a source column for balanceDuplicateColumns
    void importDuplicateColumn(const GodleyTable& srcTable, int srcCol);
    /// makes all duplicated columns consistent with \a srcTable, \a srcCol
    void balanceDuplicateColumns(const GodleyIcon& srcTable, int srcCol);

    EvalGodley evalGodley;

    // reset m_edited as the GodleyIcon constructor calls markEdited
    Minsky() {model->height=model->width=std::numeric_limits<float>::max();}

    GroupPtr model{new Group};

    void clearAllMaps();

    /// list the possible string values of an enum (for TCL)
    template <class E> void enumVals()
    {
      tclreturn r;
      for (size_t i=0; i < sizeof(enum_keysData<E>::keysData) / sizeof(EnumKey); ++i)
        r << enum_keysData<E>::keysData[i].name;
    }

    /// list of available operations
    void availableOperations() {enumVals<OperationType::Type>();}
    /// list of available variable types
    void variableTypes() {enumVals<VariableType::Type>();}

    /// return list of available asset classes
    void assetClasses() {enumVals<GodleyTable::AssetClass>();}

    /// returns reference to variable defining (ie input wired) for valueId
    VariablePtr definingVar(const std::string& valueId) const {
      return dynamic_pointer_cast<VariableBase>
        (model->findAny(&Group::items, [&](ItemPtr x) {
            auto v=dynamic_cast<VariableBase*>(x.get());
            return v && v->ports.size()>1 && v->ports[1]->wires.size()>0 && v->valueId()==valueId;
          }));
    }

//    /// create a group from items found in the current selection
    GroupPtr createGroup();
    void saveGroupAsFile(const Group&, const string& fileName) const;

    void initGodleys();

    /// select all items in rectangle bounded by \a x0, \a y0, \a x1, \a y1 
    void select(float x0, float y0, float x1, float y1);
    ///// clear selection
    void clearSelection() {currentSelection.clear();}
    /// erase items in current selection, put copy into clipboard
    void cut();
    /// copy items in current selection into clipboard
    void copy() const;
    /// paste  clipboard as a new group. @return id of nre group
    GroupPtr paste();
    void saveSelectionAsFile(const string& fileName) const;
    /// returns true if selection has any items in it
    bool itemsSelected() const {return !currentSelection.empty();}

    /// @{ override to provide clipboard handling functionality
    virtual void putClipboard(const string&) const {}
    virtual std::string getClipboard() const {return "";}
    /// @}

    /// toggle selected status of given item
    void toggleSelected(ItemType itemType, int item);

    GroupPtr insertGroupFromFile(const char* file);

    void makeVariablesConsistent();

    // runs over all ports and variables removing those not in use
    void garbageCollect();

    /// checks for presence of illegal cycles in network. Returns true
    /// if there are some
    bool cycleCheck() const;

    /// opens the log file, and writes out a header line describing
    /// names of all variables
    void openLogFile(const string&);
    /// closes log file
    void closeLogFile() {outputDataFile.reset();}

    /// construct the equations based on input data
    /// @throws ecolab::error if the data is inconsistent
    void constructEquations();
    /// evaluate the equations (stockVars.size() of them)
    void evalEquations(double result[], double t, const double vars[]);

    /// returns number of equations
    size_t numEquations() const {return 0;}//equations.size();}

    /// consistency check of the equation order. Should return
    /// true. Outputs the operation number of the invalidly ordered
    /// operation.
    bool checkEquationOrder() const;

    typedef MinskyMatrix Matrix; 
    void jacobian(Matrix& jac, double t, const double vars[]);
    
    // Runge-Kutta parameters
    double stepMin{0}; ///< minimum step size
    double stepMax{0.01}; ///< maximum step size
    int nSteps{1};     ///< number of steps per GUI update
    double epsAbs{1e-3};     ///< absolute error
    double epsRel{1e-2};     ///< relative error
    int order{4};     /// solver order: 1,2 or 4
    bool implicit{false}; /// true is implicit method used, false if explicit
    int simulationDelay{0}; /// delay in milliseconds inserted between iteration steps

    double t{0}; ///< time
    void reset(); ///<resets the variables back to their initial values
    void step();  ///< step the equations (by n steps, default 1)

    /// save to a file
    void save(const std::string& filename);
    /// load from a file
    void load(const std::string& filename);

    void exportSchema(const char* filename, int schemaLevel=1);

    /// indicate position of error on canvas
    virtual void displayErrorItem(float x, float y) const {}
    /// indicate operation item has error, if visible, otherwise contining group
    void displayErrorItem(const Item& op) const;

    /// returns operation ID for a given EvalOp. -1 if a temporary
    //    int opIdOfEvalOp(const EvalOpBase&) const;

    /// return the order in which operations are applied (for debugging purposes)
    ecolab::array<int> opOrder() const;

    /// return the AEGIS assigned version number
    static const char* minskyVersion;
    string ecolabVersion() {return VERSION;}

    unsigned maxHistory{100}; ///< maximum no. of history states to save

    /// clear history
    void clearHistory() {history.clear(); historyPtr=0;}
    /// push state onto history
    void pushHistory();
    /// called periodically to ensure history up to date
    void checkPushHistory() {if (historyPtr==history.size()) pushHistory();}

    /// push current model state onto history if it differs from previous
    bool pushHistoryIfDifferent();

    /// restore model to state \a changes ago 
    void undo(int changes=1);

    /// set a Tk image to render equations to
    void renderEquationsToImage(const char* image);

    /// Converts variable(s) named by \a name into a variable of type \a type.
    /// @throw if conversion is disallowed
    void convertVarType(const std::string& name, VariableType::Type type);

    /// returns true if any variable of name \a name has a wired input
    bool inputWired(const std::string&) const;

    /// render canvas to a cairo context
    void renderCanvas(cairo_t*) const;

    /// render canvas to a postscript file
    void renderCanvasToPS(const char* filename) const;
    /// render canvas to a PDF file
    void renderCanvasToPDF(const char* filename) const;
    /// render canvas to an SVG file
    void renderCanvasToSVG(const char* filename) const;


  };

  /// global minsky object
  Minsky& minsky();
  /// const version to help in const correctness
  inline const Minsky& cminsky() {return minsky();}
  /// RAII set the minsky object to a different one for the current scope.
  struct LocalMinsky
  {
    LocalMinsky(Minsky& m);
    ~LocalMinsky();
  };



}

#ifdef _CLASSDESC
#pragma omit pack minsky::MinskyExclude
#pragma omit unpack minsky::MinskyExclude
#pragma omit TCL_obj minsky::MinskyExclude
#pragma omit xml_pack minsky::MinskyExclude
#pragma omit xml_unpack minsky::MinskyExclude
#pragma omit xsd_generate minsky::MinskyExclude

#pragma omit xml_pack minsky::Integral
#pragma omit xml_unpack minsky::Integral

#pragma omit pack minsky::MinskyMatrix
#pragma omit unpack minsky::MinskyMatrix
#pragma omit xml_pack minsky::MinskyMatrix
#pragma omit xml_unpack minsky::MinskyMatrix
#pragma omit xsd_generate minsky::MinskyMatrix
#endif

#include "minsky.cd"
#endif
