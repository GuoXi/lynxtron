// Copyright 2026 The Lynxtron Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { EventEmitter } from 'node:events';
import { NativeImage } from './native-image';

export type MenuItemType =
  | 'normal'
  | 'separator'
  | 'submenu'
  | 'checkbox'
  | 'radio'
  | 'header'
  | 'palette';

export interface MenuItemConstructorOptions {
  id?: string;
  label?: string;
  sublabel?: string;
  toolTip?: string;
  enabled?: boolean;
  visible?: boolean;
  checked?: boolean;
  type?: MenuItemType;
  role?: string;
  accelerator?: string | null;
  acceleratorWorksWhenHidden?: boolean;
  registerAccelerator?: boolean;
  icon?: NativeImage | null;
  submenu?: Menu | MenuItemConstructorOptions[] | null;
  selector?: string;
  click?: (menuItem: MenuItem, focusedWindow: any, event: any) => void;
  after?: string[];
  before?: string[];
  afterGroupContaining?: string[];
  beforeGroupContaining?: string[];
}

export interface PopupOptions {
  window?: any;
  x?: number;
  y?: number;
  positioningItem?: number;
  callback?: () => void;
}

export declare class MenuItem {
  constructor(options?: MenuItemConstructorOptions);
  id?: string;
  label: string;
  type: MenuItemType;
  commandId: number;
  checked: boolean;
  enabled: boolean;
  visible: boolean;
  submenu?: Menu | null;
  role?: string;
  accelerator?: string | null;
  acceleratorWorksWhenHidden: boolean;
  registerAccelerator: boolean;
  icon?: NativeImage | null;
  sublabel?: string;
  toolTip?: string;
  menu?: Menu;
  selector?: string;
  groupId: number;
  click: (event: any, focusedWindow: any) => void;
  getDefaultRoleAccelerator(): string | null;
  getCheckStatus(): boolean;
  overrideProperty(name: string, defaultValue?: any): void;
  overrideReadOnlyProperty(name: string, defaultValue?: any): void;
}

// Runtime inherits EventEmitter instance methods, but not its static members.
export interface Menu extends EventEmitter {}

export declare class Menu {
  constructor();
  static getApplicationMenu(): Menu | null;
  /**
   * Sets the application menu.
   *
   * On macOS, no menu is installed automatically. The application chooses
   * when to call this method, either before or after app.whenReady(). Native
   * installation is queued on the current run loop; menu initialization can
   * block the main thread, rendering and input handling.
   *
   * For startup-sensitive applications, prefer a stage after the first window
   * has submitted its first frame and startup-critical work has completed,
   * before the application needs menu commands or accelerators.
   * app.whenReady(), ready-to-show and on-first-screen do not guarantee that
   * a frame has been submitted. A setImmediate or fixed timeout does not
   * provide that guarantee either; coordinate with the application's startup
   * lifecycle rather than treating these callbacks as frame completion.
   *
   * @param menu The menu to install. On macOS, null clears the menu items and
   * standard menu registrations without installing a default menu.
   */
  static setApplicationMenu(menu: Menu | null): void;
  static sendActionToFirstResponder(action: string): void;
  static buildFromTemplate(
    template: Array<MenuItemConstructorOptions | MenuItem>
  ): Menu;
  popup(
    options?: PopupOptions
  ): {
    lynxWindow: any;
    x: number;
    y: number;
    position: number;
  };
  closePopup(window?: any): void;
  getMenuItemById(id: string): MenuItem | null;
  append(item: MenuItem): void;
  insert(pos: number, item: MenuItem): void;
  items: MenuItem[];
}
