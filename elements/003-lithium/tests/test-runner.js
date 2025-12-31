// Lithium Test Runner
// Main test orchestration script

const { execSync } = require('child_process');
const fs = require('fs');
const path = require('path');

class TestRunner {
  constructor() {
    this.testDir = path.join(__dirname, 'tests');
    this.coverageDir = path.join(__dirname, 'coverage');
    this.reportDir = path.join(__dirname, 'reports');
    
    // Ensure directories exist
    this.ensureDirectories();
  }

  ensureDirectories() {
    [this.coverageDir, this.reportDir].forEach(dir => {
      if (!fs.existsSync(dir)) {
        fs.mkdirSync(dir, { recursive: true });
      }
    });
  }

  runTests() {
    console.log('🚀 Starting Lithium Test Suite...');
    
    try {
      // Run Mocha tests
      console.log('🧪 Running Mocha tests...');
      const testOutput = execSync('npm test', { 
        cwd: __dirname,
        encoding: 'utf8'
      });
      
      console.log('✅ Tests completed successfully');
      console.log(testOutput);
      
      return true;
    } catch (error) {
      console.error('❌ Tests failed:');
      console.error(error.stdout);
      console.error(error.stderr);
      return false;
    }
  }

  runCoverage() {
    console.log('📊 Running coverage analysis...');
    
    try {
      const coverageOutput = execSync('npm run test:coverage', { 
        cwd: __dirname,
        encoding: 'utf8'
      });
      
      console.log('✅ Coverage analysis completed');
      console.log(coverageOutput);
      
      return true;
    } catch (error) {
      console.error('❌ Coverage analysis failed:');
      console.error(error.stdout);
      console.error(error.stderr);
      return false;
    }
  }

  generateReports() {
    console.log('📈 Generating test reports...');
    
    try {
      const reportOutput = execSync('npm run test:report', { 
        cwd: __dirname,
        encoding: 'utf8'
      });
      
      console.log('✅ Reports generated');
      console.log(reportOutput);
      
      return true;
    } catch (error) {
      console.error('❌ Report generation failed:');
      console.error(error.stdout);
      console.error(error.stderr);
      return false;
    }
  }

  async runAll() {
    console.log('🎯 Running complete test suite...');
    
    const results = {
      tests: this.runTests(),
      coverage: this.runCoverage(),
      reports: this.generateReports()
    };
    
    const allPassed = Object.values(results).every(result => result === true);
    
    if (allPassed) {
      console.log('🎉 All tests passed!');
    } else {
      console.log('⚠️  Some tests failed');
    }
    
    return allPassed;
  }
}

// Run the test suite
if (require.main === module) {
  const runner = new TestRunner();
  runner.runAll().then(success => {
    process.exit(success ? 0 : 1);
  });
}

module.exports = TestRunner;