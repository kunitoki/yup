#ifndef _RIVE_DATA_BIND_HPP_
#define _RIVE_DATA_BIND_HPP_
#include "rive/component_dirt.hpp"
#include "rive/generated/data_bind/data_bind_base.hpp"
#include "rive/viewmodel/viewmodel_instance_value.hpp"
#include "rive/data_bind/context/context_value.hpp"
#include "rive/data_bind/data_context.hpp"
#include <stdio.h>
namespace rive
{
class DataBind : public DataBindBase
{
public:
    StatusCode onAddedDirty(CoreContext* context) override;
    StatusCode import(ImportStack& importStack) override;
    virtual void updateSourceBinding();
    virtual void update(ComponentDirt value);
    Core* target() const { return m_target; };
    void target(Core* value) { m_target = value; };
    virtual void bind();
    ComponentDirt dirt() { return m_Dirt; };
    void dirt(ComponentDirt value) { m_Dirt = value; };
    bool addDirt(ComponentDirt value, bool recurse);

protected:
    ComponentDirt m_Dirt = ComponentDirt::Filthy;
    Core* m_target;
    ViewModelInstanceValue* m_Source;
    std::unique_ptr<DataBindContextValue> m_ContextValue;
};
} // namespace rive

#endif