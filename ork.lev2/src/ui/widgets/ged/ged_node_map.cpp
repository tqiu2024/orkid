////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include <ork/lev2/ui/ged/ged.h>
#include <ork/lev2/ui/ged/ged_node.h>
#include <ork/lev2/ui/ged/ged_skin.h>
#include <ork/lev2/ui/ged/ged_container.h>
#include <ork/kernel/core_interface.h>
#include <ork/lev2/gfx/dbgfontman.h>
#include <ork/reflect/properties/registerX.inl>

ImplementReflectionX(ork::lev2::ged::GedMapNode, "GedMapNode");

////////////////////////////////////////////////////////////////
namespace ork::lev2::ged {
////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
KeyDecoName::KeyDecoName(const char* pkey) // precomposed name/index
{
  ArrayString<256> tempstr(pkey);
  const char* pfindcolon = strstr(tempstr.c_str(), ":");
  if (pfindcolon) {
    size_t ilen           = strlen(tempstr.c_str());
    int ipos              = (pfindcolon - tempstr.c_str());
    const char* pindexstr = pfindcolon + 1;

    size_t inumlen = ilen - ipos;

    if (inumlen) {
      miMultiIndex = atoi(pindexstr);
      char* pchar  = tempstr.begin() + ipos;
      pchar[0]     = 0;
      mActualKey.set(tempstr.c_str());
    }
  } else {
    mActualKey.set(pkey);
    miMultiIndex = 0;
  }
}
///////////////////////////////////////////////////////////////////////////////
KeyDecoName::KeyDecoName(const char* pkey, int index) // decomposed name/index
    : miMultiIndex(index)
    , mActualKey(pkey) {
}
///////////////////////////////////////////////////////////////////////////////
PropTypeString KeyDecoName::DecoratedName() const {
  PropTypeString rval;
  rval.format("%s:%d", mActualKey.c_str(), miMultiIndex);
  return rval;
}
///////////////////////////////////////////////////////////////////////////////

void GedMapNode::describeX(class_t* clazz) {
}

GedMapNode::GedMapNode(
    GedContainer* c,               //
    const char* name,              //
    const reflect::IMap* map_prop, //
    object_ptr_t obj)
    : GedItemNode(c, name, map_prop, obj)
    , mMapProp(map_prop) { //

  auto model = c->_model;

  auto enumerated_items = map_prop->enumerateElements(obj);
  for (auto e : enumerated_items) {
    auto key = e.first;
    auto val = e.second;
    std::string keyname;
    if( auto k_as_str = key.tryAs<std::string>() )
      keyname = k_as_str.value();
    else if( auto k_as_cstr = key.tryAs<const char*>() )
      keyname = k_as_cstr.value();
    else if( auto k_as_int = key.tryAs<int>() )
      keyname = FormatString("%d", k_as_int.value() );
    else
      OrkAssert(false);

    //model->createNode( name, prop, obj );
    auto item_node = model->createAbstractNode(keyname.c_str(), map_prop, obj, val );
    addChild(item_node);
  }
}

void GedMapNode::focusItem(const PropTypeString& key) {
}

bool GedMapNode::isKeyPresent(const KeyDecoName& pkey) const {
  PropTypeString deco = pkey.DecoratedName();
  bool bret           = (mMapKeys.find(deco) != mMapKeys.end());
  return bret;
}
void GedMapNode::addKey(const KeyDecoName& pkey) {
  mMapKeys.insert(std::make_pair(pkey.DecoratedName(), pkey));
}

////////////////////////////////////////////////////////////////

void GedMapNode::addItem(ui::event_constptr_t ev) {
  /*const int klabh = get_charh();
  const int kdim  = klabh - 2;

  int ibasex = (kdim + 4) * 2 + 3;

  QString qstr = GedInputDialog::getText(ev, this, 0, ibasex, 0, miW - ibasex - 6, klabh);

  std::string sstr = qstr.toStdString();
  if (sstr.length()) {
    KeyDecoName kdeca(sstr.c_str());

    if (IsKeyPresent(kdeca)) {
      if (false == IsMultiMap())
        return;
    }

    mModel.SigPreNewObject();
    GedMapIoDriver iodriver(mModel, mMapProp, GetOrkObj());
    iodriver.insert(sstr.c_str());
  }*/
}

////////////////////////////////////////////////////////////////

void GedMapNode::removeItem(ui::event_constptr_t ev) {
  /*const int klabh = get_charh();
  const int kdim  = klabh - 2;

  int ibasex = (kdim + 4) * 3 + 3;

  QString qstr = GedInputDialog::getText(ev, this, 0, ibasex, 0, miW - ibasex - 6, klabh);

  std::string sstr = qstr.toStdString();
  if (sstr.length()) {
    KeyDecoName kdec(sstr.c_str());

    if (IsKeyPresent(kdec)) {
      mModel.SigPreNewObject();
      GedMapIoDriver iodriver(mModel, mMapProp, GetOrkObj());
      iodriver.remove(kdec);
      mModel.Attach(mModel.CurrentObject());
    }
  }*/
}

////////////////////////////////////////////////////////////////

void GedMapNode::moveItem(ui::event_constptr_t ev) {
  /*
  const int klabh = get_charh();
  const int kdim  = klabh - 2;

  int ibasex = (kdim + 4) * 3 + 3;

  QString qstra = GedInputDialog::getText(ev, this, 0, ibasex, 0, miW - ibasex - 6, klabh);

  ork::msleep(100);
  QString qstrb = GedInputDialog::getText(ev, this, 0, ibasex, 0, miW - ibasex - 6, klabh);

  std::string sstra = qstra.toStdString();
  std::string sstrb = qstrb.toStdString();

  if (sstra.length() && sstrb.length()) {
    KeyDecoName kdeca(sstra.c_str());
    KeyDecoName kdecb(sstrb.c_str());

    if (IsKeyPresent(kdecb)) {
      if (false == IsMultiMap())
        return;
    }
    if (IsKeyPresent(kdeca)) {
      mModel.SigPreNewObject();

      GedMapIoDriver iodriver(mModel, mMapProp, GetOrkObj());
      iodriver.move(kdeca, sstrb.c_str());

      mModel.Attach(mModel.CurrentObject());
    }
  }*/
}

////////////////////////////////////////////////////////////////

void GedMapNode::duplicateItem(ui::event_constptr_t ev) {
  /*
  const int klabh = get_charh();
  const int kdim  = klabh - 2;

  int ibasex = (kdim + 4) * 3 + 3;

  QString qstra = GedInputDialog::getText(ev, this, 0, ibasex, 0, miW - ibasex - 6, klabh);

  ork::msleep(100);
  QString qstrb = GedInputDialog::getText(ev, this, 0, ibasex, 0, miW - ibasex - 6, klabh);

  std::string sstra = qstra.toStdString();
  std::string sstrb = qstrb.toStdString();

  if (sstra.length() && sstrb.length() && sstra != sstrb) {
    KeyDecoName kdeca(sstra.c_str());
    KeyDecoName kdecb(sstrb.c_str());

    if (IsKeyPresent(kdecb)) {
      if (false == IsMultiMap())
        return;
    }
    if (false == IsKeyPresent(kdeca)) {
      return;
    }

    mModel.SigPreNewObject();

    GedMapIoDriver iodriver(mModel, mMapProp, GetOrkObj());
    iodriver.duplicate(kdeca, sstrb.c_str());

    mModel.Attach(mModel.CurrentObject());
  }
  */
}

////////////////////////////////////////////////////////////////

void GedMapNode::importItem(ui::event_constptr_t ev) {
  /*
  const int klabh   = get_charh();
  const int kdim    = klabh - 2;
  int ibasex        = (kdim + 4) * 3 + 3;
  QString qstra     = GedInputDialog::getText(ev, this, 0, ibasex, 0, miW - ibasex - 6, klabh);
  std::string sstra = qstra.toStdString();
  if (sstra.length()) {
    KeyDecoName kdeca(sstra.c_str());
    if (IsKeyPresent(kdeca)) {
      if (false == IsMultiMap())
        return;
    }
    mModel.SigPreNewObject();
    GedMapIoDriver iodriver(mModel, mMapProp, GetOrkObj());
    iodriver.importfile(kdeca, sstra.c_str());
    mModel.Attach(mModel.CurrentObject());
  }
  */
}

////////////////////////////////////////////////////////////////

void GedMapNode::exportItem(ui::event_constptr_t ev) {
  /*
  const int klabh   = get_charh();
  const int kdim    = klabh - 2;
  int ibasex        = (kdim + 4) * 3 + 3;
  QString qstra     = GedInputDialog::getText(ev, this, 0, ibasex, 0, miW - ibasex - 6, klabh);
  std::string sstra = qstra.toStdString();
  if (sstra.length()) {
    KeyDecoName kdeca(sstra.c_str());
    if (IsKeyPresent(kdeca)) {
      GedMapIoDriver iodriver(mModel, mMapProp, GetOrkObj());
      iodriver.exportfile(kdeca, sstra.c_str());
      mModel.Attach(mModel.CurrentObject());
    }
  }
*/
}

////////////////////////////////////////////////////////////////

void GedMapNode::OnMouseDoubleClicked(ui::event_constptr_t ev) {
  const int klabh = get_charh();
  const int kdim  = klabh - 2;
  // Qt::MouseButtons Buttons = pEV->buttons();
  // Qt::KeyboardModifiers modifiers = pEV->modifiers();

  int ix = ev->miX;
  int iy = ev->miY;

  auto model = _container->_model;

  printf("GedMapNode<%p> ilx<%d> ily<%d>\n", this, ix, iy);

  if (ix >= koff && ix <= kdim && iy >= koff && iy <= kdim) // drop down
  {
    mbSingle = !mbSingle;

    ///////////////////////////////////////////
    PersistHashContext HashCtx;
    HashCtx._object   = _object;
    HashCtx._property = _property;
    auto pmap         = _container->_model->persistMapForHash(HashCtx);
    ///////////////////////////////////////////

    pmap->setValue("single", mbSingle ? "true" : "false");

    updateVisibility();
    return;
  }

  ///////////////////////////////////////

  if (mbConst == false) {
    ix -= (kdim + 4);
    if (ix >= koff && ix <= kdim && iy >= koff && iy <= kdim) // drop down
    {
      addItem(ev);
      // model.Attach(model.CurrentObject());
      printf("MAPADDITEM\n");
      model->enqueueUpdate();
      return;
    }
    ix -= (kdim + 4);
    if (ix >= koff && ix <= kdim && iy >= koff && iy <= kdim) // drop down
    {
      removeItem(ev);
      // model->Attach(model->CurrentObject());
      model->enqueueUpdate();
      return;
    }
    ix -= (kdim + 4);
    if (ix >= koff && ix <= kdim && iy >= koff && iy <= kdim) // Move Item
    {
      moveItem(ev);
      // model->Attach(model->CurrentObject());
      model->enqueueUpdate();
      return;
    }
    ix -= (kdim + 4);
    if (ix >= koff && ix <= kdim && iy >= koff && iy <= kdim) // Move Item
    {
      duplicateItem(ev);
      // model->Attach(model->CurrentObject());
      model->enqueueUpdate();
      return;
    }
  }

  ///////////////////////////////////////

  if (mbImpExp) // Import Export
  {
    ix -= (kdim + 4);
    if (ix >= koff && ix <= kdim && iy >= koff && iy <= kdim) // drop down
    {
      importItem(ev);
      // model->Attach(model->CurrentObject());
      model->enqueueUpdate();
      return;
    }
    ix -= (kdim + 4);
    if (ix >= koff && ix <= kdim && iy >= koff && iy <= kdim) // drop down
    {
      exportItem(ev);
      // model->Attach(model->CurrentObject());
      model->enqueueUpdate();
      return;
    }
  }

  ///////////////////////////////////////

  int inumitems = numChildren();

  // QMenu* pmenu = new QMenu(0);

  for (int it = 0; it < inumitems; it++) {
    auto pchild       = child(it);
    const char* pname = pchild->_propname.c_str();
    // QAction* pchildact  = pmenu->addAction(pname);
    // QString qstr(CreateFormattedString("%d", it).c_str());
    // QVariant UserData(qstr);
    // pchildact->setData(UserData);
  }
  /*QAction* pact = pmenu->exec(QCursor::pos());
  if (pact) {
    QVariant UserData = pact->data();
    std::string pname = UserData.toString().toStdString();
    int index         = 0;
    sscanf(pname.c_str(), "%d", &index);
    mItemIndex = index;
    ///////////////////////////////////////////
    PersistHashContext HashCtx;
    HashCtx.mObject     = GetOrkObj();
    HashCtx.mProperty   = GetOrkProp();
    PersistantMap* pmap = model->GetPersistMap(HashCtx);
    ///////////////////////////////////////////
    pmap->SetValue("index", CreateFormattedString("%d", mItemIndex));
    updateVisibility();
  }*/
  ///////////////////////////////////////
}

////////////////////////////////////////////////////////////////

void GedMapNode::updateVisibility() {
  /*
  int inumitems = GetNumItems();

  if (mbSingle) {
    for (int it = 0; it < inumitems; it++) {
      GetItem(it)->SetVisible(it == mItemIndex);
    }
  } else {
    for (int it = 0; it < inumitems; it++) {
      GetItem(it)->SetVisible(true);
    }
  }
  mModel.GetGedWidget()->DoResize();
  */
}

////////////////////////////////////////////////////////////////

void GedMapNode::DoDraw(Context* pTARG) {

  auto skin = _container->_activeSkin;

  const int klabh = get_charh();
  const int kdim  = klabh - 2;

  int inumind = mbConst ? 0 : 4;
  if (mbImpExp)
    inumind += 2;

  /////////////////
  // drop down box
  /////////////////

  int ioff = koff;
  int idim = (kdim);

  int dbx1 = miX + ioff;
  int dbx2 = dbx1 + idim;
  int dby1 = miY + ioff;
  int dby2 = dby1 + idim;

  int labw = this->propnameWidth();

  int ity = get_text_center_y();

  skin->DrawBgBox(this, miX, miY, miW, miH, GedSkin::ESTYLE_BACKGROUND_1);
  skin->DrawOutlineBox(this, miX, miY, miW, miH, GedSkin::ESTYLE_DEFAULT_OUTLINE);

  skin->DrawBgBox(this, miX, miY, miW, klabh, GedSkin::ESTYLE_BACKGROUND_MAPNODE_LABEL);

  if (mbSingle) {
    skin->DrawRightArrow(this, dbx1, dby1, idim, idim, GedSkin::ESTYLE_BUTTON_OUTLINE);
    skin->DrawLine(this, dbx1 + 1, dby1, dbx1 + 1, dby2, GedSkin::ESTYLE_BUTTON_OUTLINE);
  } else {
    skin->DrawDownArrow(this, dbx1, dby1, idim, idim, GedSkin::ESTYLE_BUTTON_OUTLINE);
    skin->DrawLine(this, dbx1, dby1 + 1, dbx2, dby1 + 1, GedSkin::ESTYLE_BUTTON_OUTLINE);
  }

  dbx1 += (idim + 4);
  dbx2 = dbx1 + idim;

  if (mbConst == false) {
    int idimh = idim >> 1;

    skin->DrawOutlineBox(this, dbx1, dby1, idim, idim, GedSkin::ESTYLE_BUTTON_OUTLINE);
    skin->DrawLine(this, dbx1 + idimh, dby1, dbx1 + idimh, dby1 + idim, GedSkin::ESTYLE_BUTTON_OUTLINE);
    skin->DrawLine(this, dbx1, dby1 + idimh, dbx2, dby1 + idimh, GedSkin::ESTYLE_BUTTON_OUTLINE);

    dbx1 += (idim + 4);
    dbx2 = dbx1 + idim;

    skin->DrawOutlineBox(this, dbx1, dby1, idim, idim, GedSkin::ESTYLE_BUTTON_OUTLINE);
    skin->DrawLine(this, dbx1, dby1 + idimh, dbx2, dby1 + idimh, GedSkin::ESTYLE_BUTTON_OUTLINE);

    dbx1 += (idim + 4);
    dbx2     = dbx1 + idim;
    int dbxc = dbx1 + idimh;

    skin->DrawOutlineBox(this, dbx1, dby1, idim, idim, GedSkin::ESTYLE_BUTTON_OUTLINE);
    skin->DrawText(this, dbx1, dby1 - 1, "R");

    dbx1 += (idim + 4);
    dbx2 = dbx1 + idim;
    dbxc = dbx1 + idimh;
    skin->DrawOutlineBox(this, dbx1, dby1, idim, idim, GedSkin::ESTYLE_BUTTON_OUTLINE);
    skin->DrawText(this, dbx1, dby1 - 1, "D");

    dbx1 += (idim + 4);
    dbx2 = dbx1 + idim;
  }

  if (mbImpExp) {
    int idimh = idim >> 1;

    skin->DrawOutlineBox(this, dbx1, dby1, idim, idim, GedSkin::ESTYLE_BUTTON_OUTLINE);
    skin->DrawText(this, dbx1, dby1 - 1, "I");

    dbx1 += (idim + 4);
    dbx2     = dbx1 + idim;
    int dbxc = dbx1 + idimh;

    skin->DrawOutlineBox(this, dbx1, dby1, idim, idim, GedSkin::ESTYLE_BUTTON_OUTLINE);
    skin->DrawText(this, dbx1, dby1 - 1, "O");

    dbx1 += (idim + 4);
    dbx2 = dbx1 + idim;
    // skin->DrawLine( this, dbx1, dby1, dbxc, dby1+idimh, GedSkin::ESTYLE_BUTTON_OUTLINE );
    // skin->DrawLine( this, dbxc, dby1+idimh, dbx2, dby1, GedSkin::ESTYLE_BUTTON_OUTLINE );
  }
  skin->DrawText(this, dbx1, ity - 2, _propname.c_str());
}

////////////////////////////////////////////////////////////////

} // namespace ork::lev2::ged
