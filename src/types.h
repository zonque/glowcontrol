#ifndef _TYPES_H_
#define _TYPES_H_

#include "intl.h"

typedef struct _Scene       Scene;
typedef struct _SceneEditor SceneEditor;
typedef struct _Step        Step;
typedef struct _StepEditor  StepEditor;
typedef struct _Project     Project;

typedef struct _XMLWriter   XMLWriter;
typedef struct _XMLParser   XMLParser;

enum 
  {
    COLUMN_ID,
    COLUMN_ICON,
    COLUMN_STRING,
    COLUMN_DATA,
    COLUMN_IS_EDITABLE,
    LAST_COL
  };


#endif /* _TYPES_H_ */
