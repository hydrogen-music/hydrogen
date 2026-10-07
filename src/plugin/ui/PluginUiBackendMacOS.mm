/*
 * Hydrogen
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

// macOS backend of PluginUiWindow (proposal 0006 TU2.2/TU2.3): an NSView
// child on the host-provided parent view plus a CAMetalLayer-backed Metal
// context, driven frame-by-frame from idle(). CI-gated: no macOS build
// host locally — the AppVeyor macOS job compiles and runs the smoke
// against this (skipping when no window server/Metal device exists).
// Metal is the recorded de-risk item ("not well tested" upstream, proposal
// 0006 TU2.4) — the spike gate checks it on real hardware.

#include "PluginUiBackend.h"

#include <imgui_impl_metal.h>

#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <cstdint>
#include <new>
#include <memory>

namespace H2Core {

namespace {

class PluginUiBackendMacOS : public PluginUiBackend {
public:
	PluginUiBackendMacOS( std::uintptr_t parentNativeHandle, int width,
						  int height )
		: m_parent( reinterpret_cast<NSView*>( parentNativeHandle ) ) {
		if ( m_parent == nil ) {
			return;
		}

		// Headless CI (no window server) yields no device: the backend
		// reports invalid and the smoke skips with a message.
		m_device = MTLCreateSystemDefaultDevice();
		if ( m_device == nil ) {
			return;
		}

		m_view = [[NSView alloc]
			initWithFrame: NSMakeRect( 0, 0, width, height )];
		if ( m_view == nil ) {
			return;
		}
		m_view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

		// The view owns the layer after setLayer: (it retains it), so our
		// alloc reference is released right away; the layer is reached
		// through the view from here on.
		CAMetalLayer* layer = [[CAMetalLayer alloc] init];
		layer.device = m_device;
		layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
		layer.drawableSize = CGSizeMake( width, height );
		m_view.layer = layer;
		[layer release];
		m_view.wantsLayer = YES;

		[m_parent addSubview: m_view];

		m_commandQueue = [m_device newCommandQueue];
		if ( m_commandQueue == nil ) {
			return;
		}

		m_valid = true;
	}

	~PluginUiBackendMacOS() override {
		if ( m_view != nil ) {
			[m_view removeFromSuperview];
			[m_view release];
		}
		if ( m_commandQueue != nil ) {
			[m_commandQueue release];
		}
		if ( m_device != nil ) {
			[m_device release];
		}
	}

	bool valid() const override {
		return m_valid;
	}

	std::uintptr_t nativeHandle() const override {
		return reinterpret_cast<std::uintptr_t>( m_view );
	}

	double scaleFactor() const override {
		// The parent view is the one living in a host window; our own view
		// may not be in one yet at smoke time.
		if ( m_scaleFactor <= 0.0 && m_parent != nil ) {
			NSWindow* window = [m_parent window];
			if ( window != nil ) {
				m_scaleFactor = [window backingScaleFactor];
			}
		}
		return m_scaleFactor > 0.0 ? m_scaleFactor : 1.0;
	}

	void getSize( int& width, int& height ) const override {
		if ( m_view != nil ) {
			const NSSize size = [m_view bounds].size;
			m_width = static_cast<int>( size.width );
			m_height = static_cast<int>( size.height );
		}
		width = m_width;
		height = m_height;
	}

	unsigned inputEventCount() const override {
		// Input arrives through the responder chain into the NSView;
		// wiring the observation is UI-4 scope.
		return 0;
	}

	void pumpEvents() override {
		// The host's event loop delivers events; nothing to pump here.
	}

	bool initRenderBackend() override {
		return ImGui_ImplMetal_Init( m_device );
	}

	void shutdownRenderBackend() override {
		ImGui_ImplMetal_Shutdown();
	}

	void beginFrame() override {
		CAMetalLayer* layer = (CAMetalLayer*)m_view.layer;
		// drawableSize is in pixels; bounds are in points.
		const NSSize bounds = [m_view bounds].size;
		const double scale = scaleFactor();
		layer.drawableSize = CGSizeMake(
			bounds.width * scale, bounds.height * scale );
		ImGui_ImplMetal_NewFrame();
	}

	void renderFrame( ImDrawData* data ) override {
		@autoreleasepool {
			CAMetalLayer* layer = (CAMetalLayer*)m_view.layer;
			id<CAMetalDrawable> drawable = [layer nextDrawable];
			if ( drawable == nil ) {
				return; // no drawable this frame (e.g. occluded) — skip
			}
			MTLRenderPassDescriptor* pass =
				[MTLRenderPassDescriptor renderPassDescriptor];
			pass.colorAttachments[ 0 ].texture = [drawable texture];
			pass.colorAttachments[ 0 ].loadAction = MTLLoadActionClear;
			pass.colorAttachments[ 0 ].storeAction = MTLStoreActionStore;
			pass.colorAttachments[ 0 ].clearColor =
				MTLClearColorMake( 0.1, 0.1, 0.12, 1.0 );
			id<MTLCommandBuffer> commandBuffer = [m_commandQueue commandBuffer];
			id<MTLRenderCommandEncoder> encoder =
				[commandBuffer renderCommandEncoderWithDescriptor: pass];
			ImGui_ImplMetal_RenderDrawData( data, commandBuffer, encoder );
			[encoder endEncoding];
			[commandBuffer presentDrawable: drawable];
			[commandBuffer commit];
		}
	}

	void endFrame() override {
		// Metal presented inside renderFrame (the drawable and command
		// buffer are autoreleased); nothing to release here.
	}

private:
	NSView* m_parent = nil;
	NSView* m_view = nil;
	id<MTLDevice> m_device = nil;
	id<MTLCommandQueue> m_commandQueue = nil;
	mutable int m_width = 0;
	mutable int m_height = 0;
	mutable double m_scaleFactor = 0.0;
	bool m_valid = false;
};

} // namespace

std::unique_ptr<PluginUiBackend> createPluginUiBackend(
	std::uintptr_t parentNativeHandle, int width, int height ) {
	auto* backend = new ( std::nothrow ) PluginUiBackendMacOS(
		parentNativeHandle, width, height );
	if ( backend == nullptr ) {
		return nullptr;
	}
	return std::unique_ptr<PluginUiBackend>( backend );
}

} // namespace H2Core
