/*
 * InvalidSessionInfo.java
 *
 * Copyright (C) 2009-13 by RStudio, Inc.
 *
 * Unless you have received this program directly from RStudio pursuant
 * to the terms of a commercial license agreement with RStudio, then
 * this program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */
package org.rstudio.studio.client.application.model;

import com.google.gwt.core.client.JavaScriptObject;


public class InvalidSessionInfo extends JavaScriptObject
{
   protected InvalidSessionInfo() {}

   public native final String getScopePath() /*-{ 
      return this.scope_path; 
   }-*/;
   
   public native final String getSessionProject() /*-{ 
      return this.project; 
   }-*/;
   
   public native final String getSessionProjectId() /*-{ 
      return this.id; 
   }-*/;
 }
