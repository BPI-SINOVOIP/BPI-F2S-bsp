#ifndef __GL2SW_INT_H__
#define __GL2SW_INT_H__


#ifndef INTERRUPT_IMMEDIATELY
void rx_do_tasklet(unsigned long data);
void tx_do_tasklet(unsigned long data);
#endif
#ifdef RX_POLLING
int rx_poll(struct napi_struct *napi, int budget);
#endif
irqreturn_t ethernet_tx_interrupt(int irq, void *dev_id);
irqreturn_t ethernet_rx_interrupt(int irq, void *dev_id);
int l2sw_get_irq(struct platform_device *pdev, struct l2sw_common *comm);
int l2sw_request_irq(struct platform_device *pdev, struct l2sw_common *comm, struct net_device *net_dev);

#endif

