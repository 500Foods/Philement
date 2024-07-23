unit UnitMain;

interface

uses
  System.SysUtils, System.Classes, JS, Web, WEBLib.Graphics, WEBLib.Controls,
  WEBLib.Forms, WEBLib.Dialogs, Vcl.Controls, Vcl.StdCtrls, WEBLib.StdCtrls;

type
  TForm1 = class(TWebForm)
    WebLabel1: TWebLabel;
    procedure WebFormCreate(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  Form1: TForm1;

implementation

{$R *.dfm}

procedure TForm1.WebFormCreate(Sender: TObject);
begin

  {$IFNDEF WIN32} asm {

        const BACKGROUND = {
            ROTATE_SPEED: 0.1,  // degrees per second
            FPS: 3,            // frames per second
            MIN_SCALE: 1,      // minimum scale factor
            MAX_SCALE: 1.4,    // maximum scale factor
            SCALE_DURATION: 600, // seconds for a complete scale cycle
            pattern: document.getElementById('bgHexagons'),
            rect: document.getElementById('bgRect'),
            angle: 0,
            scalePhase: 0
        };

        function background_smoothstep(min, max, value) {
            const x = Math.max(0, Math.min(1, (value - min) / (max - min)));
            return x * x * (3 - 2 * x);  // Smoothing function
        }

        function background_update(timestamp) {
            // Update rotation
            BACKGROUND.angle += BACKGROUND.ROTATE_SPEED / BACKGROUND.FPS;
            if (BACKGROUND.angle >= 360) BACKGROUND.angle -= 360;

            // Update scale
            BACKGROUND.scalePhase += 1 / (BACKGROUND.SCALE_DURATION * BACKGROUND.FPS);
            if (BACKGROUND.scalePhase > 1) BACKGROUND.scalePhase -= 1;
            const scaleValue = BACKGROUND.MIN_SCALE + (BACKGROUND.MAX_SCALE - BACKGROUND.MIN_SCALE) * background_smoothstep(0, 1, Math.abs(2 * BACKGROUND.scalePhase - 1));

            // Apply transformations
            BACKGROUND.pattern.setAttribute('patternTransform', `rotate(${BACKGROUND.angle}, 25, 21.7) scale(${scaleValue})`);

            setTimeout(() => requestAnimationFrame(background_update), 1000 / BACKGROUND.FPS);
        }

        // Add resize handler for responsiveness
        window.addEventListener('resize', () => {
            const svg = document.getElementById('background');
            svg.setAttribute('viewBox', `0 0 ${window.innerWidth} ${window.innerHeight}`);
        });

        requestAnimationFrame(background_update);

  } end; {$ENDIF}

end;

end.