#ifndef CXXNET_LAYER_SOFTMAX_LAYER_INL_HPP_
#define CXXNET_LAYER_SOFTMAX_LAYER_INL_HPP_

#include <mshadow/tensor.h>
#include "./layer.h"

namespace cxxnet {
namespace layer {
/*! \brief softmax layer, do softmax transformation during forward */
template<typename xpu>
class SoftmaxLayer: public ILayer<xpu> {
 public:
  SoftmaxLayer(const LabelInfo *label_info) {
    this->plabelinfo = label_info;
  }
  virtual ~SoftmaxLayer(void) {}
  virtual void SetParam(const char *name, const char *val) {
    if (!strcmp(name, "batch_size")) batch_size = atoi(val);
  }
  virtual void InitConnection(const std::vector<Node<xpu>*> &nodes_in,
                              const std::vector<Node<xpu>*> &nodes_out,
                              ConnectState<xpu> *p_cstate) {
    utils::Check(nodes_in.size() == 1 && nodes_out.size() == 1,
                 "SoftmaxLayer: only support 1-1 connection");
    utils::Check(nodes_in[0] == nodes_out[0], "SoftmaxLayer is an self-loop Layer");
    tnode.Resize(nodes_in[0]->mat().shape);
  }
  virtual void Forward(bool is_train,
                       const std::vector<Node<xpu>*> &nodes_in,
                       const std::vector<Node<xpu>*> &nodes_out,
                       ConnectState<xpu> *p_cstate) {
    mshadow::Softmax(nodes_in[0]->mat(), nodes_in[0]->mat());
  }
  virtual void Backprop(bool prop_grad,
                        const std::vector<Node<xpu>*> &nodes_in,
                        const std::vector<Node<xpu>*> &nodes_out,
                        ConnectState<xpu> *p_cstate) {
    using namespace mshadow::expr;
    // do computation in CPU for simplicity, since this is not bottle neck
    tnode.Resize(nodes_in[0]->mat().shape);
    mshadow::Copy(tnode, nodes_in[0]->mat());
    for (mshadow::index_t i = 0; i < plabelinfo->batch_size; ++i) {
      index_t k = static_cast<index_t>(plabelinfo->labels[i]);
      tnode[i][k] -= 1.0f;
    }
    mshadow::Copy(nodes_in[0]->mat(), tnode);
    // scale gradient by dividing global batch size
    nodes_in[0]->mat() *= (1.0f / batch_size);
  }
  
 protected:
  /*!
   * \brief global batch_size set by user, this 
   *        is not necessarily the batch_size in plabelinfo, since a batch can be divided 
   *        into subbatch to layers in different devices
   */
  int batch_size;
  /*! \brief temp space for notde*/
  mshadow::TensorContainer<cpu,2> tnode;
  /*! \brief reference to label information */
  const LabelInfo *plabelinfo;
};
}  // namespace layer
}  // namespace cxxnet
#endif  // LAYER_SOFTMAX_LAYER_INL_HPP_