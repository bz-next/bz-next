/* bzflag
 * Copyright 1993-1999, Chris Schoeneman
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named LICENSE that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/* SGIPlatformFactory:
 *	Factory for SGI Irix platform stuff.
 */

#ifndef BZF_SGIPLATFORM_FACTORY_H
#define	BZF_SGIPLATFORM_FACTORY_H

#include "PlatformFactory.h"

class SGIPlatformFactory : public PlatformFactory {
  public:
			SGIPlatformFactory();
			~SGIPlatformFactory();

    BzfDisplay*		createDisplay(const char* name, const char*);
    BzfVisual*		createVisual(const BzfDisplay*);
    BzfWindow*		createWindow(const BzfDisplay*, BzfVisual*);

  private:
			SGIPlatformFactory(const SGIPlatformFactory&);
    SGIPlatformFactory&	operator=(const SGIPlatformFactory&);

    BzfMedia*		createMedia();
};

#endif // BZF_SGIPLATFORM_FACTORY_H
