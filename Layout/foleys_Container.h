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

#pragma once

namespace foleys
{

/**
 The Container is a Decorator, that can hold multiple Components.
 In the editor it is seen as "View". With the setting "display"
 the layout strategy can be chosen.
 */
class Container : public Decorator
{
public:
    enum class Layout
    {
        Contents,
        FlexBox
    };

    Container (MagicBuilder& builder, juce::ValueTree node);

    void addChildItem (std::unique_ptr<Decorator> child);

    /**
     This switches this node and all it's descendents in the edit
     mode, which means, the components don't react, but instead you
     can move them around.
     */
    void setEditMode (bool shouldEdit) override;

    void resized() override;

    bool isContainer() const override { return true; }

    juce::FlexBox flexBox;

    Layout layout = Layout::FlexBox;

private:

    std::vector<std::unique_ptr<Decorator>> children;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Container)
};

} // namespace foleys
