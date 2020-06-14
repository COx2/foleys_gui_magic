/*
 ==============================================================================
    Copyright (c) 2019 Foleys Finest Audio Ltd. - Daniel Walz
    All rights reserved.

    Redistribution and use in source and binary forms, with or without modification,
    are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
    INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
    LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
    OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
    OF THE POSSIBILITY OF SUCH DAMAGE.
 ==============================================================================
 */

namespace foleys
{


MagicGUIBuilder::MagicGUIBuilder (MagicGUIState& state)
  : magicState (state)
{
    config = juce::ValueTree (IDs::magic);

    updateStylesheet();
}

Stylesheet& MagicGUIBuilder::getStylesheet()
{
    return stylesheet;
}

juce::ValueTree& MagicGUIBuilder::getConfigTree()
{
    return config;
}

juce::ValueTree MagicGUIBuilder::getGuiRootNode()
{
    return config.getChildWithName (IDs::view);
}

std::unique_ptr<GuiItem> MagicGUIBuilder::createGuiItem (const juce::ValueTree& node)
{
    if (node.getType() == IDs::view)
    {
        auto item = std::make_unique<Container>(*this, node);
        item->updateInternal();
        item->createSubComponents();
        return item;
    }

    auto factory = factories.find (node.getType());
    if (factory != factories.end())
    {
        auto item = factory->second (*this, node);
        item->updateInternal();
        return item;
    }

    DBG ("No factory for: " << node.getType().toString());
    return {};
}

void MagicGUIBuilder::updateStylesheet()
{
    auto stylesNode = config.getOrCreateChildWithName (IDs::styles, &undo);
    if (stylesNode.getNumChildren() == 0)
        stylesNode.appendChild (magicState.createDefaultStylesheet(), &undo);

    auto selectedName = stylesNode.getProperty (IDs::selected, {}).toString();
    if (selectedName.isNotEmpty())
    {
        auto style = stylesNode.getChildWithProperty (IDs::name, selectedName);
        stylesheet.setStyle (style);
    }
    else
    {
        stylesheet.setStyle (stylesNode.getChild (0));
    }

    stylesheet.updateStyleClasses();
    stylesheet.updateValidRanges();
}

void MagicGUIBuilder::clearGUI()
{
    auto guiNode = config.getOrCreateChildWithName (IDs::view, &undo);
    guiNode.removeAllChildren (&undo);
    guiNode.removeAllProperties (&undo);

    updateComponents();
}

void MagicGUIBuilder::resetToDefaultGUI()
{
    auto guiNode = config.getOrCreateChildWithName (IDs::view, &undo);
    guiNode.removeAllChildren (&undo);
    guiNode.removeAllProperties (&undo);
    guiNode.copyPropertiesAndChildrenFrom (magicState.createDefaultGUITree(), &undo);

    updateComponents();
}

void MagicGUIBuilder::showOverlayDialog (std::unique_ptr<juce::Component> dialog)
{
    if (parent == nullptr)
        return;

    overlayDialog = std::move (dialog);
    parent->addAndMakeVisible (overlayDialog.get());

    updateLayout();
}

void MagicGUIBuilder::closeOverlayDialog()
{
    overlayDialog.reset();
}

void MagicGUIBuilder::setConfigTree (const juce::ValueTree& gui)
{
    if (gui.isValid() == false)
        return;

    if (config.isValid())
    {
        auto parentNode = config.getParent();
        parentNode.removeChild (config, nullptr);
        config = gui;
        if (parentNode.isValid())
            parentNode.appendChild (config, nullptr);
    }
    else
    {
        config = gui;
    }

    undo.clearUndoHistory();
    updateComponents();
}

void MagicGUIBuilder::setConfigTree (const char* data, const int dataSize)
{
    juce::String text (data, dataSize);
    auto gui = juce::ValueTree::fromXml (text);
    setConfigTree (gui);
}

void MagicGUIBuilder::createGUI (juce::Component& parentToUse)
{
    parent = &parentToUse;

    updateComponents();

#if FOLEYS_SHOW_GUI_EDITOR_PALLETTE
    if (magicToolBox.get() != nullptr)
        magicToolBox->stateWasReloaded();
#endif
}

void MagicGUIBuilder::updateComponents()
{
    if (parent == nullptr)
        return;

    updateStylesheet();

    if (config.getChildWithName (IDs::view).isValid() == false)
        config.appendChild (magicState.createDefaultGUITree(), &undo);

    auto rootNode = config.getOrCreateChildWithName (IDs::view, &undo);

    root = createGuiItem (rootNode);
    parent->addAndMakeVisible (root.get());

    root->setBounds (parent->getLocalBounds());

#if FOLEYS_SHOW_GUI_EDITOR_PALLETTE6
    if (root.get() != nullptr)
        root->setEditMode (editMode);
#endif
}

void MagicGUIBuilder::updateLayout()
{
    if (parent == nullptr)
        return;

    if (root.get() != nullptr)
    {
        if (! stylesheet.setMediaSize (parent->getWidth(), parent->getHeight()))
        {
            stylesheet.updateValidRanges();
            root->updateInternal();
        }

        if (root->getBounds() == parent->getLocalBounds())
            root->updateLayout();
        else
            root->setBounds (parent->getLocalBounds());
    }

    if (overlayDialog)
    {
        if (overlayDialog->getBounds() == parent->getLocalBounds())
            overlayDialog->resized();
        else
            overlayDialog->setBounds (parent->getLocalBounds());
    }

    parent->repaint();
}

void MagicGUIBuilder::registerFactory (juce::Identifier type, std::unique_ptr<GuiItem>(*factory)(MagicGUIBuilder& builder, const juce::ValueTree&))
{
    if (factories.find (type) != factories.cend())
    {
        // You tried to add two factories with the same type name!
        // That cannot work, the second factory will be ignored.
        jassertfalse;
        return;
    }

    factories [type] = factory;
}

juce::StringArray MagicGUIBuilder::getFactoryNames() const
{
    juce::StringArray names { IDs::view.toString() };

    names.ensureStorageAllocated (int (factories.size()));
    for (const auto& f : factories)
        names.add (f.first.toString());

    return names;
}

void MagicGUIBuilder::registerLookAndFeel (juce::String name, std::unique_ptr<juce::LookAndFeel> lookAndFeel)
{
    stylesheet.registerLookAndFeel (name, std::move (lookAndFeel));
}

void MagicGUIBuilder::registerJUCELookAndFeels()
{
    stylesheet.registerLookAndFeel ("LookAndFeel_V1", std::make_unique<juce::LookAndFeel_V1>());
    stylesheet.registerLookAndFeel ("LookAndFeel_V2", std::make_unique<juce::LookAndFeel_V2>());
    stylesheet.registerLookAndFeel ("LookAndFeel_V3", std::make_unique<juce::LookAndFeel_V3>());
    stylesheet.registerLookAndFeel ("LookAndFeel_V4", std::make_unique<juce::LookAndFeel_V4>());
    stylesheet.registerLookAndFeel ("FoleysFinest", std::make_unique<LookAndFeel>());
    stylesheet.registerLookAndFeel ("Skeuomorphic", std::make_unique<Skeuomorphic>());
}

juce::var MagicGUIBuilder::getStyleProperty (const juce::Identifier& name, const juce::ValueTree& node) const
{
    return stylesheet.getStyleProperty (name, node);
}

void MagicGUIBuilder::removeStyleClassReferences (juce::ValueTree tree, const juce::String& name)
{
    if (tree.hasProperty (IDs::styleClass))
    {
        const auto separator = " ";
        auto strings = juce::StringArray::fromTokens (tree.getProperty (IDs::styleClass).toString(), separator, "");
        strings.removeEmptyStrings (true);
        strings.removeString (name);
        tree.setProperty (IDs::styleClass, strings.joinIntoString (separator), &undo);
    }

    for (auto child : tree)
        removeStyleClassReferences (child, name);
}

juce::StringArray MagicGUIBuilder::getColourNames (juce::Identifier type)
{
    juce::ValueTree node (type);
    if (auto item = createGuiItem (node))
        return item->getColourNames();

    return {};
}

void MagicGUIBuilder::populatePropertiesMenu (juce::ComboBox& comboBox) const
{
    return magicState.populatePropertiesMenu (comboBox);
}

juce::PopupMenu MagicGUIBuilder::createParameterMenu() const
{
    return magicState.createParameterMenu();
}

juce::PopupMenu MagicGUIBuilder::createPropertiesMenu() const
{
    // FIXME
    return {};
}

juce::PopupMenu MagicGUIBuilder::createTriggerMenu() const
{
    return magicState.createTriggerMenu();
}

juce::var MagicGUIBuilder::getPropertyDefaultValue (juce::Identifier property) const
{
    // flexbox
    if (property == IDs::flexDirection) return IDs::flexDirRow;
    if (property == IDs::flexWrap)      return IDs::flexNoWrap;
    if (property == IDs::flexAlignContent) return IDs::flexStretch;
    if (property == IDs::flexAlignItems) return IDs::flexStretch;
    if (property == IDs::flexJustifyContent) return IDs::flexStart;
    if (property == IDs::flexAlignSelf) return IDs::flexStretch;
    if (property == IDs::flexOrder) return 0;
    if (property == IDs::flexGrow) return 1.0;
    if (property == IDs::flexShrink) return 1.0;
    if (property == IDs::minWidth) return 0.0;
    if (property == IDs::minHeight) return 0.0;
    if (property == IDs::display) return IDs::flexbox;

    if (property == IDs::captionPlacement) return "centred-top";
    if (property == IDs::lookAndFeel) return "FoleysFinest";

    if (property == juce::Identifier ("font-size")) return 12.0;

    return {};
}

void MagicGUIBuilder::changeListenerCallback (juce::ChangeBroadcaster*)
{
    if (root.get() != nullptr)
        root->updateInternal();

    updateLayout();
}

MagicGUIState& MagicGUIBuilder::getMagicState()
{
    return magicState;
}

juce::UndoManager& MagicGUIBuilder::getUndoManager()
{
    return undo;
}

#if FOLEYS_SHOW_GUI_EDITOR_PALLETTE

void MagicGUIBuilder::attachToolboxToWindow (juce::Component& window)
{
    juce::Component::SafePointer<juce::Component> reference (&window);

    juce::MessageManager::callAsync ([&, reference]
                                     {
                                         if (reference != nullptr)
                                             magicToolBox = std::make_unique<ToolBox>(reference->getTopLevelComponent(), *this);
                                     });
}

void MagicGUIBuilder::setEditMode (bool shouldEdit)
{
    editMode = shouldEdit;

    if (parent == nullptr)
        return;

    if (root.get() != nullptr)
        root->setEditMode (shouldEdit);

    if (shouldEdit == false)
        setSelectedNode (juce::ValueTree());

    parent->repaint();
}

bool MagicGUIBuilder::isEditModeOn() const
{
    return editMode;
}

void MagicGUIBuilder::setSelectedNode (const juce::ValueTree& node)
{
    if (selectedNode != node)
    {
        selectedNode = node;
        if (magicToolBox.get() != nullptr)
            magicToolBox->setSelectedNode (selectedNode);

        if (parent != nullptr)
            parent->repaint();
    }
}

const juce::ValueTree& MagicGUIBuilder::getSelectedNode() const
{
    return selectedNode;
}

void MagicGUIBuilder::draggedItemOnto (juce::ValueTree dragged, juce::ValueTree target, int index)
{
    if (dragged == target)
        return;

    undo.beginNewTransaction();

    auto targetParent  = target.getParent();
    auto draggedParent = dragged.getParent();

    if (draggedParent.isValid())
        draggedParent.removeChild (dragged, &undo);

    if (targetParent.isValid() != false && index < 0)
        index = targetParent.indexOf (target);

    if (target.getType() == IDs::view)
        target.addChild (dragged, index, &undo);
    else
        targetParent.addChild (dragged, index, &undo);
}

ToolBox& MagicGUIBuilder::getMagicToolBox()
{
    // The magicToolBox should always be present!
    // This method wouldn't be included, if
    // FOLEYS_SHOW_GUI_EDITOR_PALLETTE was 0
    jassert (magicToolBox.get() != nullptr);

    return *magicToolBox;
}

#endif


} // namespace foleys
