#ifndef __EHCI_PLATFORM_H
#define __EHCI_PLATFORM_H

#ifdef CONFIG_PM
extern struct dev_pm_ops ehci_sunplus_pm_ops;
#endif

extern int ehci_platform_probe(struct platform_device *dev);

extern int ehci_platform_remove(struct platform_device *dev);

#endif




