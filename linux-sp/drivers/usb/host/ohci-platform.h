#ifndef __OHCI_PLATFORM_H
#define __OHCI_PLATFORM_H


#ifdef CONFIG_PM
extern struct dev_pm_ops ohci_sunplus_pm_ops;
#endif

extern int ohci_platform_probe(struct platform_device *dev);

extern int ohci_platform_remove(struct platform_device *dev);


#endif

