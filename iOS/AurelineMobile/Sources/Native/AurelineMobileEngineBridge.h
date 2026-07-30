#import <AudioToolbox/AudioToolbox.h>
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface AurelineMobileEngineBridge : NSObject

- (void)prepareWithSampleRate:(double)sampleRate NS_SWIFT_NAME(prepare(sampleRate:));
- (void)noteOn:(int)note velocity:(int)velocity NS_SWIFT_NAME(noteOn(_:velocity:));
- (void)noteOff:(int)note NS_SWIFT_NAME(noteOff(_:));
- (void)setPitchBend:(double)value;
- (void)setPitchBendRange:(double)value NS_SWIFT_NAME(setPitchBendRange(_:));
- (void)setModWheel:(double)value;
- (void)setSustainPedal:(BOOL)down NS_SWIFT_NAME(setSustainPedal(_:));
- (void)panic;
- (void)resetPatch;
- (NSArray<NSString*>*)factoryPresetNames;
- (void)loadFactoryPreset:(int)index NS_SWIFT_NAME(loadFactoryPreset(_:));
- (void)setParameter:(NSString*)identifier value:(double)value;
- (double)parameterValue:(NSString*)identifier;
- (NSDictionary<NSString*, NSNumber*>*)patchSnapshot;
- (void)applyPatchSnapshot:(NSDictionary<NSString*, NSNumber*>*)snapshot;
- (void)configureDrumKit:(NSArray<NSDictionary<NSString*, NSNumber*>*>*)snapshots;
- (void)renderLeft:(float*)left right:(float*)right frames:(int)frames;
- (void)renderToAudioBufferList:(AudioBufferList*)audioBufferList frames:(int)frames;
- (NSData*)scopeSnapshotData NS_SWIFT_NAME(scopeSnapshotData());
- (double)currentLFOValue NS_SWIFT_NAME(currentLFOValue());

@end

NS_ASSUME_NONNULL_END
